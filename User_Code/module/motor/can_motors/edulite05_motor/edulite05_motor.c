//
// Created by Lenovo on 2026/11/7.
//
#include "edulite05_motor.h"
#include "bsp_log.h"
#include "cmsis_os.h"
#include "daemon.h"
#include "general_def.h"
#include "memory.h"
#include "motor_def.h"
#include "stdlib.h"
#include "string.h"
#include "user_lib.h"

static uint8_t idx;
static EDUMotorInstance *edu_motor_instance[EDU_MOTOR_CNT]; // 实例登记册: 每格存一个实例指针,初始全 NULL
static TaskHandle_t edu_task_handle[EDU_MOTOR_CNT];

/* 两个用于将uint值和float值进行映射的函数,在设定发送值和解析反馈值时使用 */
static uint16_t float_to_uint(float x, float x_min, float x_max, uint8_t bits)
{
    float span = x_max - x_min;
    float offset = x_min;
    return (uint16_t)((x - offset) * ((float)((1 << bits) - 1)) / span);
}

static float uint_to_float(int x_int, float x_min, float x_max, int bits)
{
    float span = x_max - x_min;
    float offset = x_min;
    return ((float)x_int) * span / ((float)((1 << bits) - 1)) + offset;
}

/**
 * @brief 发送一次性控制帧(使能/停止/标零)
 * @note EL05 与 DM 的区别: 命令类型在扩展帧 ID 的 bit28~24,不在数据区
 */
static void EduLite05SetMode(EDUMotor_Cmd_e cmd, EDUMotorInstance *motor)
{
    FDCANInstance *can = motor->motor_can_instance;

    // 数据区: 命令帧全 0;标零帧 Byte[0]=1(对应手册通信类型6)
    memset(can->tx_buff, 0, 8);
    if (cmd == EDU_CMD_ZERO)
    {
        can->tx_buff[0] = 1;
    }

    // 29位扩展帧ID:
    //    bit28~24 = 通信类型(3使能/4停止/6标零)
    //    bit23~8  = 主机 CAN_ID(EDU_MASTER_ID = 0xFD)
    //    bit7~0   = 电机 CAN_ID
    can->txconf.Identifier = ((uint32_t)cmd << 24) | ((uint32_t)EDU_MASTER_ID << 8) | motor->motor_id;
    can->txconf.IdType = FDCAN_EXTENDED_ID; // 必须覆盖: bsp 的 CANRegister 默认标准帧

    CANTransmit(can, 1);
}

/**
 * @brief EL05 反馈帧解析回调(通信类型2)
 * 与 DM 的三点区别:
 *  ① 数据区全 16bit 大端,没有 12bit 交错解码
 *  ② 故障/模式状态/电机ID 在扩展帧 ID 里,不在数据区
 *  ③ 只有一个温度,且单位是 ℃×10,要 ÷10
 */
static void EduLite05Decode(FDCANInstance *motor_can)
{
    uint16_t tmp;
    uint8_t *rxbuff = motor_can->rx_buff;
    EDUMotorInstance *motor = (EDUMotorInstance *)motor_can->id;
    EDU_Motor_Measure_s *measure = &(motor->measure);
    uint32_t ext_id = motor_can->rx_ext_id; // 完整 29 位反馈帧 ID(bsp 扩展帧分发时保存)

    DaemonReload(motor->motor_daemon);          // 喂狗: 收到反馈 = 电机在线
    measure->last_position = measure->position; // 存上次位置(多圈计数用)

    // 从扩展帧 ID 解析身份/故障/模式状态
    measure->id = (uint8_t)((ext_id >> 8) & 0xFF);    // bit8~15  电机 CAN_ID
    measure->fault = (ext_id >> 16) & 0x3F;           // bit16~21 故障位
    measure->mode_status = (ext_id >> 22) & 0x03;     // bit22~23 模式状态(0复位/1标定/2运行)

    // 数据区 8 字节全大端解析(手册通信类型2)
    tmp = (uint16_t)((rxbuff[0] << 8) | rxbuff[1]);
    measure->position = uint_to_float(tmp, EL05_P_MIN, EL05_P_MAX, 16); // rad
    tmp = (uint16_t)((rxbuff[2] << 8) | rxbuff[3]);
    measure->velocity = uint_to_float(tmp, EL05_V_MIN, EL05_V_MAX, 16); // rad/s
    tmp = (uint16_t)((rxbuff[4] << 8) | rxbuff[5]);
    measure->torque = uint_to_float(tmp, EL05_T_MIN, EL05_T_MAX, 16);   // Nm
    tmp = (uint16_t)((rxbuff[6] << 8) | rxbuff[7]);
    measure->temperature = (float)tmp / 10.0f;                          // ℃×10 → ℃

    // TODO: 多圈计数——新版固件反馈 -4π~4π 周期循环,
    //       参照 dji_motor 的"半圈跳变检测"实现,total_round 字段已预留
}

static void EduLite05LostCallback(void *motor_ptr)
{
    EDUMotorInstance *motor = (EDUMotorInstance *)motor_ptr;
    LOGWARNING("[edulite05_motor] motor lost, id [%d]", motor->motor_id);
    motor->stop_flag = MOTOR_STOP;
    EduLite05SetMode(EDU_CMD_STOP, motor); // 失联 = 立即发停止帧,防止关节失控
}

void EDUMotorCaliEncoder(EDUMotorInstance *motor)
{
    EduLite05SetMode(EDU_CMD_ZERO, motor);
    DWT_Delay(0.001); // 1ms(注意: DWT_Delay 单位是秒!)
}

EDUMotorInstance *EDUMotorInit(Motor_Init_Config_s *config)
{
    if (idx >= EDU_MOTOR_CNT)
    {
        LOGERROR("[edulite05_motor] instance exceeded max [%d]", EDU_MOTOR_CNT);
        return NULL;
    }
    EDUMotorInstance *motor = (EDUMotorInstance *)malloc(sizeof(EDUMotorInstance));
    memset(motor, 0, sizeof(EDUMotorInstance));

    // 基本信息(约定: config->fdcan_init_config.tx_id = 电机 CAN_ID)
    motor->ctrl_mode = EDU_CTRL_MIT;
    motor->motor_id = config->fdcan_init_config.tx_id;

    // 控制设置 + 三环 PID
    motor->motor_settings = config->controller_setting_init_config;
    PIDInit(&motor->current_PID, &config->controller_param_init_config.current_PID);
    PIDInit(&motor->speed_PID, &config->controller_param_init_config.speed_PID);
    PIDInit(&motor->angle_PID, &config->controller_param_init_config.angle_PID);
    motor->other_angle_feedback_ptr = config->controller_param_init_config.other_angle_feedback_ptr;
    motor->other_speed_feedback_ptr = config->controller_param_init_config.other_speed_feedback_ptr;
    motor->speed_feedforward_ptr = config->controller_param_init_config.speed_feedforward_ptr;
    motor->current_feedforward_ptr = config->controller_param_init_config.current_feedforward_ptr;

    // CAN 注册
    config->fdcan_init_config.can_module_callback = EduLite05Decode;
    config->fdcan_init_config.id = motor;
    // rx_id 不用上层填: 反馈帧过滤键 = 类型2(bit28~24) + 电机ID(bit8~15) + 主机ID(bit7~0)
    config->fdcan_init_config.rx_id = (2u << 24) | ((uint32_t)motor->motor_id << 8) | EDU_MASTER_ID;
    // 扩展帧 + 掩码过滤(忽略每帧变化的故障/状态位)
    config->fdcan_init_config.filter_mask = EDU_RX_FILTER_MASK;
    config->fdcan_init_config.id_type = FDCAN_EXTENDED_ID;
    motor->motor_can_instance = CANRegister(&config->fdcan_init_config);

    // 守护注册(1kHz 反馈,10 个周期 ≈ 10ms 没收到算失联)
    Daemon_Init_Config_s conf = {
        .callback = EduLite05LostCallback,
        .owner_id = motor,
        .reload_count = 10,
    };
    motor->motor_daemon = DaemonRegister(&conf);

    // 使能(EL05 必须发类型3帧,不发电机不响应)
    EDUMotorEnable(motor);
    // 注意: 不在这里标零!EL05 标零会覆盖机械零点参考,留给上层标定流程显式调用

    edu_motor_instance[idx++] = motor;
    return motor;
}

void EDUMotorSetRef(EDUMotorInstance *motor, float ref)
{
    motor->pid_ref = ref; // 纯力控目标力矩(Nm)
}

void EDUMotorSetMITParam(EDUMotorInstance *motor, float p, float v, float kp, float kd, float t_ff)
{
    // 直通运控模式: 电机端自带 PD
    // TODO: 当前 EduLite05Task 只实现了纯力控,此信箱待直通模式启用后在任务组帧时消费
    motor->send_mailbox.position_des = float_to_uint(p, EL05_P_MIN, EL05_P_MAX, 16);
    motor->send_mailbox.velocity_des = float_to_uint(v, EL05_V_MIN, EL05_V_MAX, 16);
    motor->send_mailbox.Kp = float_to_uint(kp, EL05_KP_MIN, EL05_KP_MAX, 16);
    motor->send_mailbox.Kd = float_to_uint(kd, EL05_KD_MIN, EL05_KD_MAX, 16);
    motor->send_mailbox.t_ff = float_to_uint(t_ff, EL05_T_MIN, EL05_T_MAX, 16);
}

void EDUMotorEnable(EDUMotorInstance *motor)
{
    motor->stop_flag = MOTOR_ENALBED;
    EduLite05SetMode(EDU_CMD_ENABLE, motor); // 真正发使能帧(每次调用都发)
    DWT_Delay(0.001);                        // 1ms
}

void EDUMotorStop(EDUMotorInstance *motor)
{
    motor->stop_flag = MOTOR_STOP;
    EduLite05SetMode(EDU_CMD_STOP, motor);   // 真正发停止帧
    DWT_Delay(0.001);
}

void EDUMotorOuterLoop(EDUMotorInstance *motor, Closeloop_Type_e type)
{
    motor->motor_settings.outer_loop_type = type; // 预留: 板端串级 PID 的外层闭环选择
}

void EDUMotorSetCtrlMode(EDUMotorInstance *motor, EDUMotor_Ctrl_Mode_e mode)
{
    motor->ctrl_mode = mode; // 预留: 以后切电流/速度/位置模式时用
}

/**
 * @brief 每电机控制任务(参照 DM 的 DMMotorTask)
 * @note 纯力控: p=v=Kp=Kd=0, 只下发前馈力矩 t_ff(进扩展帧 ID 的 bit23~8)
 */
void EduLite05Task(void *argument)
{
    EDUMotorInstance *motor = (EDUMotorInstance *)argument;
    FDCANInstance *can = motor->motor_can_instance;
    uint16_t t_ff;

    while (1)
    {
        // 纯力控: 力矩 = pid_ref;停止时强制 0
        t_ff = float_to_uint(motor->pid_ref, EL05_T_MIN, EL05_T_MAX, 16);
        if (motor->stop_flag == MOTOR_STOP)
        {
            t_ff = float_to_uint(0, EL05_T_MIN, EL05_T_MAX, 16);
        }

        // 数据区: 位置/速度/Kp/Kd 编码为"零位"(16bit 大端)
        uint16_t p_zero = float_to_uint(0, EL05_P_MIN, EL05_P_MAX, 16);
        uint16_t v_zero = float_to_uint(0, EL05_V_MIN, EL05_V_MAX, 16);
        can->tx_buff[0] = (uint8_t)(p_zero >> 8);
        can->tx_buff[1] = (uint8_t)(p_zero & 0xFF);
        can->tx_buff[2] = (uint8_t)(v_zero >> 8);
        can->tx_buff[3] = (uint8_t)(v_zero & 0xFF);
        can->tx_buff[4] = 0; // Kp = 0
        can->tx_buff[5] = 0;
        can->tx_buff[6] = 0; // Kd = 0
        can->tx_buff[7] = 0;

        // ID: 类型1(bit28~24) + 前馈力矩 t_ff(bit23~8) + 电机ID(bit7~0),扩展帧
        can->txconf.Identifier = (1u << 24) | ((uint32_t)t_ff << 8) | motor->motor_id;
        can->txconf.IdType = FDCAN_EXTENDED_ID;
        CANTransmit(can, 1);

        osDelay(1); // 1kHz
    }
}

/**
 * @brief 为所有已注册实例创建控制任务(必须在 RTOS 调度器启动后调用)
 */
void EDUMotorControlInit()
{
    if (!idx)
    {
        return;
    }
    for (size_t i = 0; i < idx; i++)
    {
        // 用 FreeRTOS 原生 API(CMSIS-RTOS V2 封装层不支持动态任务名)
        if (xTaskCreate(EduLite05Task, "EduLite05Task", 256, edu_motor_instance[i], osPriorityNormal,
                        &edu_task_handle[i]) != pdPASS)
        {
            LOGERROR("[edulite05_motor] Failed to create motor thread for motor %d", i);
        }
    }
}
