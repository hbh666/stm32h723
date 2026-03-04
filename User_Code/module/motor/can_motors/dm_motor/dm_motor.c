#include "dm_motor.h"
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
static DMMotorInstance *dm_motor_instance[DM_MOTOR_CNT];
static TaskHandle_t dm_task_handle[DM_MOTOR_CNT];
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

static void DMMotorSetMode(DMMotor_Mode_e cmd, DMMotorInstance *motor)
{
    memset(motor->motor_can_instance->tx_buff, 0xff, 7);  // 发送电机指令的时候前面7bytes都是0xff
    motor->motor_can_instance->tx_buff[7] = (uint8_t)cmd; // 最后一位是命令id
    CANTransmit(motor->motor_can_instance, 1);
}

static void DMMotorDecode(FDCANInstance *motor_can)
{
    uint16_t tmp; // 用于暂存解析值,稍后转换成float数据,避免多次创建临时变量
    uint8_t *rxbuff = motor_can->rx_buff;
    DMMotorInstance *motor = (DMMotorInstance *)motor_can->id;
    DM_Motor_Measure_s *measure = &(motor->measure); // 将can实例中保存的id转换成电机实例的指针

    DaemonReload(motor->motor_daemon);

    measure->last_position = measure->position;

    // 区分一控四模式和MIT模式的反馈解析
    if (motor->ctrl_mode == DM_CTRL_ONE_TO_FOUR)
    {
        // 标识符为0x300+电机ID时的解析逻辑
        // D[0], D[1] 为位置高/低8位，范围0-8191对应一圈位置
        tmp = (uint16_t)((rxbuff[0] << 8) | rxbuff[1]);
        measure->position = (float)tmp;

        // D[2], D[3] 为速度高/低8位，单位rpm放大一百倍
        int16_t vel_tmp = (int16_t)((rxbuff[2] << 8) | rxbuff[3]);
        measure->velocity = (float)vel_tmp / 100.0f;

        // D[4], D[5] 为扭矩电流高/低8位，单位mA
        int16_t torq_tmp = (int16_t)((rxbuff[4] << 8) | rxbuff[5]);
        measure->torque = (float)torq_tmp / 1000.0f;

        // D[6] 为电机线圈温度
        measure->T_Mos = (float)rxbuff[6];

        // D[7] 为错误状态
        measure->state = rxbuff[7];
    }
    else 
    {
        // 原MIT反馈解析逻辑
        tmp = (uint16_t)((rxbuff[1] << 8) | rxbuff[2]);
        measure->position = uint_to_float(tmp, DM_P_MIN, DM_P_MAX, 16);

        tmp = (uint16_t)((rxbuff[3] << 4) | rxbuff[4] >> 4);
        measure->velocity = uint_to_float(tmp, DM_V_MIN, DM_V_MAX, 12);

        tmp = (uint16_t)(((rxbuff[4] & 0x0f) << 8) | rxbuff[5]);
        measure->torque = uint_to_float(tmp, DM_T_MIN, DM_T_MAX, 12);

        measure->T_Mos = (float)rxbuff[6];
        measure->T_Rotor = (float)rxbuff[7];
    }
}

static void DMMotorLostCallback(void *motor_ptr)
{
}
void DMMotorCaliEncoder(DMMotorInstance *motor)
{
    DMMotorSetMode(DM_CMD_ZERO_POSITION, motor);
    DWT_Delay(0.1);
}
DMMotorInstance *DMMotorInit(Motor_Init_Config_s *config)
{
    DMMotorInstance *motor = (DMMotorInstance *)malloc(sizeof(DMMotorInstance));
    memset(motor, 0, sizeof(DMMotorInstance));
    
    // 默认初始化为MIT模式
    motor->ctrl_mode = DM_CTRL_MIT;

    motor->motor_settings = config->controller_setting_init_config;
    PIDInit(&motor->current_PID, &config->controller_param_init_config.current_PID);
    PIDInit(&motor->speed_PID, &config->controller_param_init_config.speed_PID);
    PIDInit(&motor->angle_PID, &config->controller_param_init_config.angle_PID);
    motor->other_angle_feedback_ptr = config->controller_param_init_config.other_angle_feedback_ptr;
    motor->other_speed_feedback_ptr = config->controller_param_init_config.other_speed_feedback_ptr;

    config->fdcan_init_config.can_module_callback = DMMotorDecode;
    config->fdcan_init_config.id = motor;
    motor->motor_can_instance = CANRegister(&config->fdcan_init_config);

    Daemon_Init_Config_s conf = {
        .callback = DMMotorLostCallback,
        .owner_id = motor,
        .reload_count = 10,
    };
    motor->motor_daemon = DaemonRegister(&conf);

    DMMotorEnable(motor);
    DMMotorSetMode(DM_CMD_MOTOR_MODE, motor);
    DWT_Delay(0.1);
    DMMotorCaliEncoder(motor);
    DWT_Delay(0.1);
    dm_motor_instance[idx++] = motor;
    return motor;
}

void DMMotorSetRef(DMMotorInstance *motor, float ref)
{
    motor->pid_ref = ref;
}

void DMMotorEnable(DMMotorInstance *motor)
{
    motor->stop_flag = MOTOR_ENALBED;
}

void DMMotorStop(DMMotorInstance *motor)//不使用使能模式是因为需要收到反馈
{
    motor->stop_flag = MOTOR_STOP;
}

void DMMotorOuterLoop(DMMotorInstance *motor, Closeloop_Type_e type)
{
    motor->motor_settings.outer_loop_type = type;
}

void DMMotorSetCtrlMode(DMMotorInstance *motor, DMMotor_Ctrl_Mode_e mode)
{
    motor->ctrl_mode = mode;
}

// 一控四下发指令：支持1帧控制4个电机，由外部统一调用，不要在DMMotorTask中高频调用此函数
void DMMotorSendOneToFourGroup(FDCANInstance *can_instance, uint8_t group, float i1, float i2, float i3, float i4)
{
    // 根据电机ID组配置对应的报文ID：[1,4]为0x3FE, [5,8]为0x4FE
    uint32_t tx_id = (group == 1) ? 0x3FE : 0x4FE;

    // 控制电流为标幺值，采用力位混控i_des相同的16位映射机制进行量化
    uint16_t cur1 = float_to_uint(i1, DM_T_MIN, DM_T_MAX, 16);
    uint16_t cur2 = float_to_uint(i2, DM_T_MIN, DM_T_MAX, 16);
    uint16_t cur3 = float_to_uint(i3, DM_T_MIN, DM_T_MAX, 16);
    uint16_t cur4 = float_to_uint(i4, DM_T_MIN, DM_T_MAX, 16);

    uint32_t old_id = can_instance->tx_id;
    can_instance->tx_id = tx_id;

    // 数据段填充：先低8位，再高8位
    can_instance->tx_buff[0] = (uint8_t)(cur1 & 0xFF);
    can_instance->tx_buff[1] = (uint8_t)(cur1 >> 8);
    can_instance->tx_buff[2] = (uint8_t)(cur2 & 0xFF);
    can_instance->tx_buff[3] = (uint8_t)(cur2 >> 8);
    can_instance->tx_buff[4] = (uint8_t)(cur3 & 0xFF);
    can_instance->tx_buff[5] = (uint8_t)(cur3 >> 8);
    can_instance->tx_buff[6] = (uint8_t)(cur4 & 0xFF);
    can_instance->tx_buff[7] = (uint8_t)(cur4 >> 8);
    
    CANTransmit(can_instance, 1);
    
    can_instance->tx_id = old_id; // 恢复旧有配置
}

// 一控四模式下特殊清零指令
void DMMotorSetZeroOneToFour(FDCANInstance *can_instance, uint16_t target_can_id)
{
    uint32_t old_id = can_instance->tx_id;
    can_instance->tx_id = 0x7FF; // 零点设置特殊指令报文ID

    // 依序填入CANID和固定魔法字
    can_instance->tx_buff[0] = (uint8_t)(target_can_id & 0xFF);
    can_instance->tx_buff[1] = (uint8_t)(target_can_id >> 8);
    can_instance->tx_buff[2] = 0x55;
    can_instance->tx_buff[3] = 0x50;
    can_instance->tx_buff[4] = 0x00;
    can_instance->tx_buff[5] = 0x00;
    can_instance->tx_buff[6] = 0x00;
    can_instance->tx_buff[7] = 0x00;
    
    CANTransmit(can_instance, 1);
    can_instance->tx_id = old_id;
}


//@Todo: MIT模式目前只实现了力控，更多位控PID等请自行添加
void DMMotorTask(void *argument)
{
    float  pid_ref, set;
    DMMotorInstance *motor = (DMMotorInstance *)argument;
    Motor_Control_Setting_s *setting = &motor->motor_settings;
    DMMotor_Send_s motor_send_mailbox;
    while (1)
    {
        // 若当前实例设为了一控四模式，不应由单独的电机Task发送报文
        // 需要在用户外部的任务里定期调用 DMMotorSendOneToFourGroup()
        if (motor->ctrl_mode == DM_CTRL_ONE_TO_FOUR)
        {
            osDelay(2);
            continue;
        }

        pid_ref = motor->pid_ref;
        
        set = pid_ref;
        if (setting->motor_reverse_flag == MOTOR_DIRECTION_REVERSE)
            set *= -1;
       
        LIMIT_MIN_MAX(set, DM_T_MIN, DM_T_MAX);
        motor_send_mailbox.position_des = float_to_uint(0, DM_P_MIN, DM_P_MAX, 16);
        motor_send_mailbox.velocity_des = float_to_uint(0, DM_V_MIN, DM_V_MAX, 12);
        motor_send_mailbox.torque_des = float_to_uint(pid_ref, DM_T_MIN, DM_T_MAX, 12);
        motor_send_mailbox.Kp = 0;
        motor_send_mailbox.Kd = 0;

        if(motor->stop_flag == MOTOR_STOP)
            motor_send_mailbox.torque_des = float_to_uint(0, DM_T_MIN, DM_T_MAX, 12);

        motor->motor_can_instance->tx_buff[0] = (uint8_t)(motor_send_mailbox.position_des >> 8);
        motor->motor_can_instance->tx_buff[1] = (uint8_t)(motor_send_mailbox.position_des);
        motor->motor_can_instance->tx_buff[2] = (uint8_t)(motor_send_mailbox.velocity_des >> 4);
        motor->motor_can_instance->tx_buff[3] = (uint8_t)(((motor_send_mailbox.velocity_des & 0xF) << 4) | (motor_send_mailbox.Kp >> 8));
        motor->motor_can_instance->tx_buff[4] = (uint8_t)(motor_send_mailbox.Kp);
        motor->motor_can_instance->tx_buff[5] = (uint8_t)(motor_send_mailbox.Kd >> 4);
        motor->motor_can_instance->tx_buff[6] = (uint8_t)(((motor_send_mailbox.Kd & 0xF) << 4) | (motor_send_mailbox.torque_des >> 8));
        motor->motor_can_instance->tx_buff[7] = (uint8_t)(motor_send_mailbox.torque_des);

        CANTransmit(motor->motor_can_instance, 1);

        osDelay(2);
    }
}

void DMMotorControlInit()
{
    // 遍历所有电机实例,创建任务
    if (!idx)
        return;
    
    // 注意：CMSIS-RTOS V2的osThreadDef不支持动态生成的名称
    // 我们需要为每个电机创建独立的线程定义或使用不同的方法
    
    // 方案1：使用循环和预定义的线程定义（如果电机数量固定）
    // 这里改为直接使用FreeRTOS原生API创建线程，更加灵活可靠
    
    for (size_t i = 0; i < idx; i++)
    {
        // 使用FreeRTOS原生API创建线程
        // 参数：线程函数、线程名称、堆栈大小、参数、优先级、线程句柄
        if (xTaskCreate(DMMotorTask, "DMMotorTask", 128, dm_motor_instance[i], osPriorityNormal, &dm_task_handle[i]) != pdPASS)
        {
            LOGERROR("[DM_Motor] Failed to create motor thread for motor %d", i);
        }
    }
}