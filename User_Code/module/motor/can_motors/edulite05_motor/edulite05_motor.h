//
// Created by Lenovo on 2026/11/7.
//

#ifndef TRONONEH7_SCAFFOLD_EDULITE05_MOTOR_H
#define TRONONEH7_SCAFFOLD_EDULITE05_MOTOR_H

#include "bsp_fdcan.h"
#include "pid.h"
#include "motor_def.h"
#include "daemon.h"

#define EDU_MOTOR_CNT 4

/* 主机 CAN_ID:EL05 所有一次性命令帧(类型3/4/6)的扩展帧 ID bit23~8 填此值 */
#define EDU_MASTER_ID 0xFD

/* 反馈帧过滤掩码: 比对 bit28~24(类型=2) + bit8~15(电机ID) + bit7~0(主机ID),
 * 忽略 bit16~23(故障位/模式状态,每帧都在变) */
#define EDU_RX_FILTER_MASK 0x1F00FFFFu

/* EL05 运控模式量程(必须与电机固件一致,见 EL05 使用说明书):
 * 位置 ±12.57rad, 速度 ±50rad/s, 力矩 ±6Nm, Kp 0~500, Kd 0~5 */
#define EL05_P_MIN  (-12.57f)
#define EL05_P_MAX   12.57f
#define EL05_V_MIN  (-50.0f)
#define EL05_V_MAX   50.0f
#define EL05_T_MIN  (-6.0f)
#define EL05_T_MAX   6.0f
#define EL05_KP_MIN  0.0f
#define EL05_KP_MAX  500.0f
#define EL05_KD_MIN  0.0f
#define EL05_KD_MAX  5.0f

/* 扩展帧 ID 结构:29 位 = [bit28~24 通信类型][bit23~8 数据区2][bit7~0 目标地址] */
#define EDU_ID_TYPE_MASK   0x1F000000u   // 通信类型
#define EDU_ID_DATA2_MASK  0x00FFFF00u   // 数据区2
#define EDU_ID_TARGET_MASK 0x000000FFu   // 目标地址

/* 电机模式状态(反馈帧 ID bit22~23) */
typedef enum
{
    EDU_MODE_RESET = 0, // Reset 模式[复位]
    EDU_MODE_CALI  = 1, // Cali 模式[标定]
    EDU_MODE_MOTOR = 2, // Motor 模式[运行]
} EDU_Motor_Mode_Status_e;

/* 故障位(反馈帧 ID bit16~21 解析出 fault 值后按位判断,手册通信类型2) */
#define EDU_FAULT_UNDERVOLTAGE   (1u << 0) // bit16: 欠压故障
#define EDU_FAULT_PHASE_CURRENT  (1u << 1) // bit17: 三相电流故障
#define EDU_FAULT_OVERTEMP       (1u << 2) // bit18: 过温
#define EDU_FAULT_ENCODER        (1u << 3) // bit19: 磁编码故障
#define EDU_FAULT_STALL          (1u << 4) // bit20: 堵转过载故障
#define EDU_FAULT_UNCALIBRATED   (1u << 5) // bit21: 未标定

/**
 * @brief 电机测量值(由通信类型2反馈帧解析)
 * 数据区 8 字节大端: [0:1]角度 [2:3]角速度 [4:5]力矩 [6:7]温度(℃×10)
 */
typedef struct
{
    uint8_t id;          // 电机 CAN_ID(反馈帧 ID bit8~15)
    uint32_t fault;      // 故障位集合(反馈帧 ID bit16~21,按 EDU_FAULT_* 掩码判断)
    uint8_t mode_status; // 模式状态(反馈帧 ID bit22~23,EDU_Motor_Mode_Status_e)

    float position;      // 当前角度 rad(±12.57)
    float last_position; // 上一次采样的角度(多圈计数用)
    float velocity;      // 当前角速度 rad/s(±50)
    float torque;        // 当前力矩 Nm(±6)
    float temperature;   // 温度 ℃(反馈原始值 ÷10)
    int32_t total_round; // 累计圈数(预留,需自行实现 ±2π 跳变检测)
} EDU_Motor_Measure_s;

/**
 * @brief 运控指令发送信箱(通信类型1,编码后的 uint 值)
 * 数据区 8 字节大端: [0:1]位置 [2:3]速度 [4:5]Kp [6:7]Kd
 * 注意:t_ff 不在此结构体的数据区,组帧时拼进扩展帧 ID 的 bit23~8!
 */
typedef struct
{
    uint16_t position_des; // 目标角度(16bit 编码)
    uint16_t velocity_des; // 目标角速度(16bit 编码)
    uint16_t Kp;           // 刚度 0~500(16bit 编码)
    uint16_t Kd;           // 阻尼 0~5(16bit 编码)
    uint16_t t_ff;         // 前馈力矩 ±6Nm(16bit 编码,进扩展帧 ID bit23~8)
} EDU_Motor_Send_s;

//电机控制模式
typedef enum
{
    EDU_CTRL_MIT = 0, // 运控模式(上电默认)
    // TODO: 电流/速度/位置模式需通信类型18设置 runmode(3/2/5),暂未实现
} EDUMotor_Ctrl_Mode_e;

/**
 * @brief 电机实例(每个电机一个,由 EDUMotorInit 分配)
 */
typedef struct
{
    EDU_Motor_Measure_s measure;
    Motor_Control_Setting_s motor_settings;
    PIDInstance  current_PID;
    PIDInstance  speed_PID;
    PIDInstance angle_PID;
    float *other_angle_feedback_ptr;
    float *other_speed_feedback_ptr;
    float *speed_feedforward_ptr;
    float *current_feedforward_ptr;
    float pid_ref;                       // 力控目标力矩(EDUMotorSetRef 写入)
    EDU_Motor_Send_s send_mailbox;       // 运控参数信箱(EDUMotorSetMITParam 写入,直通模式用)
    Motor_Working_Type_e stop_flag;      // 启停标志
    FDCANInstance *motor_can_instance;   // CAN 收发通道(CANRegister 返回)
    Daemon_Instance* motor_daemon;       // 守护实例(失联自动停止)

    EDUMotor_Ctrl_Mode_e ctrl_mode;      // 控制模式
    uint8_t motor_id;                    // 电机 CAN_ID(扩展帧 ID bit7~0)
}EDUMotorInstance;

/* 一次性命令帧类型(扩展帧 ID bit28~24) */
typedef enum
{
    EDU_CMD_ENABLE = 3, // 通信类型3: 使能运行(上电后必须发,电机才响应运控指令)
    EDU_CMD_STOP   = 4, // 通信类型4: 停止运行(data 全 0;Byte[0]=1 时=清故障)
    EDU_CMD_ZERO   = 6, // 通信类型6: 设置机械零位(Byte[0]=1)
}EDUMotor_Cmd_e;

/**
 * @brief 注册一个 EL05 关节电机
 * @param config 电机初始化配置(复用通用 Motor_Init_Config_s,与 DM/DJI 电机一致)
 *       约定: config->fdcan_init_config.tx_id = 电机 CAN_ID(出厂默认可用类型7修改)
 *              config->fdcan_init_config.rx_id 由内部按反馈帧格式计算,无需填写
 * @return 电机实例指针,失败返回 NULL
 */
EDUMotorInstance *EDUMotorInit(Motor_Init_Config_s *config);

/**
 * @brief 设定目标力矩(纯力控模式: 任务内 p=v=Kp=Kd=0, 仅下发 t_ff)
 */
void EDUMotorSetRef(EDUMotorInstance *motor, float ref);

/**
 * @brief 直通运控模式: 一次设定 p/v/Kp/Kd/t_ff 五个运控参数(电机端自带 PD)
 */
void EDUMotorSetMITParam(EDUMotorInstance *motor, float p, float v, float kp, float kd, float t_ff);

/**
 * @brief 修改板端闭环的最外层目标(板端串级 PID 用,预留)
 */
void EDUMotorOuterLoop(EDUMotorInstance *motor, Closeloop_Type_e closeloop_type);

/**
 * @brief 使能电机(发送通信类型3使能帧)
 */
void EDUMotorEnable(EDUMotorInstance *motor);

/**
 * @brief 停止电机(发送通信类型4停止帧,并置 stop_flag)
 */
void EDUMotorStop(EDUMotorInstance *motor);

/**
 * @brief 将电机当前位置设置为机械零位(发送通信类型6)
 * @attention 不会在 Init 中自动调用,由上层在标定流程中显式调用
 */
void EDUMotorCaliEncoder(EDUMotorInstance *motor);

/**
 * @brief 切换电机控制模式(预留: 以后切电流/速度/位置模式时用)
 */
void EDUMotorSetCtrlMode(EDUMotorInstance *motor, EDUMotor_Ctrl_Mode_e mode);

/**
 * @brief 为所有已注册实例创建控制任务(必须在 RTOS 调度器启动后调用)
 */
void EDUMotorControlInit();

#endif // TRONONEH7_SCAFFOLD_EDULITE05_MOTOR_H
