#ifndef DM_MOTOR_H
#define DM_MOTOR_H
#include <stdint.h>
#include "bsp_fdcan.h"
#include "pid.h"
#include "motor_def.h"
#include "daemon.h"

#define DM_MOTOR_CNT 4

#define DM_P_MIN  (-12.5f)
#define DM_P_MAX  12.5f
#define DM_V_MIN  (-45.0f)
#define DM_V_MAX  45.0f
#define DM_T_MIN  (-18.0f)
#define DM_T_MAX   18.0f

// 新增：电机控制模式枚举
typedef enum {
    DM_CTRL_MIT = 0,//MIT模式，默认的单电机控制模式
    DM_CTRL_ONE_TO_FOUR = 1, // 一控四模式
} DMMotor_Ctrl_Mode_e;

typedef struct 
{
    uint8_t id;
    uint8_t state;
    float velocity;
    float last_position;
    float position;
    float torque;
    float T_Mos;
    float T_Rotor;
    int32_t total_round;
}DM_Motor_Measure_s;

typedef struct
{
    uint16_t position_des;
    uint16_t velocity_des;
    uint16_t torque_des;
    uint16_t Kp;
    uint16_t Kd;
}DMMotor_Send_s;

typedef struct 
{
    DM_Motor_Measure_s measure;
    Motor_Control_Setting_s motor_settings;
    PIDInstance current_PID;
    PIDInstance speed_PID;
    PIDInstance angle_PID;
    float *other_angle_feedback_ptr;
    float *other_speed_feedback_ptr;
    float *speed_feedforward_ptr;
    float *current_feedforward_ptr;
    float pid_ref;
    Motor_Working_Type_e stop_flag;
    FDCANInstance *motor_can_instance;
    Daemon_Instance* motor_daemon;
    uint32_t lost_cnt;
    
    // 新增：当前电机的控制模式标志
    DMMotor_Ctrl_Mode_e ctrl_mode;
}DMMotorInstance;

typedef enum
{
    DM_CMD_MOTOR_MODE = 0xfc,   // 使能,会响应指令
    DM_CMD_RESET_MODE = 0xfd,   // 停止
    DM_CMD_ZERO_POSITION = 0xfe, // 将当前的位置设置为编码器零位
    DM_CMD_CLEAR_ERROR = 0xfb // 清除电机过热错误
}DMMotor_Mode_e;

DMMotorInstance *DMMotorInit(Motor_Init_Config_s *config);

void DMMotorSetRef(DMMotorInstance *motor, float ref);

void DMMotorOuterLoop(DMMotorInstance *motor,Closeloop_Type_e closeloop_type);

void DMMotorEnable(DMMotorInstance *motor);

void DMMotorStop(DMMotorInstance *motor);
void DMMotorCaliEncoder(DMMotorInstance *motor);
void DMMotorControlInit();

// 新增：设置电机控制模式
void DMMotorSetCtrlMode(DMMotorInstance *motor, DMMotor_Ctrl_Mode_e mode);

// 新增：一控四模式下，单控制帧发送4个电机的电流
void DMMotorSendOneToFourGroup(FDCANInstance *can_instance, uint8_t group, float i1, float i2, float i3, float i4);

// 新增：一控四模式下的零点设置
void DMMotorSetZeroOneToFour(FDCANInstance *can_instance, uint16_t target_can_id);

#endif // !DMMOTOR