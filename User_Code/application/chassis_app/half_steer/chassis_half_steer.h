#ifndef CHASSIS_HALF_STEER_H
#define CHASSIS_HALF_STEER_H

#include "stdint.h"
#include "dji_motor.h"
#include "lk_motor.h" // 假设存在对应的C接口头文件
#include "pid.h"
#include "chassis_ctrl.h" // 包含底盘控制相关的通用定义

// 定义底盘控制命令结构体 (如果 chassis_ctrl.h 中未定义，请在此定义或确保通用)
#ifndef CHASSIS_CTRL_CMD_DEFINED
#define CHASSIS_CTRL_CMD_DEFINED
typedef enum
{
    CHASSIS_ZERO_FORCE = 0,    // 无力/急停
    CHASSIS_NO_FOLLOW,         // 不跟随/自由移动
    CHASSIS_FOLLOW_GIMBAL_YAW, // 跟随云台Yaw
    CHASSIS_ROTATE,            // 小陀螺/自旋
} Chassis_Mode_e;

typedef struct
{
    float vx;           // 前后速度 (m/s)
    float vy;           // 左右速度 (m/s)
    float wz;           // 旋转角速度 (rad/s 或 对应单位)
    float offset_angle; // 底盘与云台的夹角 (度)
    Chassis_Mode_e chassis_mode;
} Chassis_Ctrl_Cmd_s;
#endif

// 定义底盘反馈数据结构体
typedef struct
{
    float vx;
    float vy;
    float wz;
    // float real_angle; // 预留
} Chassis_Upload_Data_s;

// 半舵轮底盘对象结构体
typedef struct
{
    // 轮毂电机实例 (驱动) - LK9015
    LKMotorInstance *motor_drive_rf; // 右前
    LKMotorInstance *motor_drive_lb; // 左后

    // 舵向电机实例 (转向) - GM6020
    DJIMotorInstance *motor_steer_rf; // 右前舵
    DJIMotorInstance *motor_steer_lb; // 左后舵

    // PID实例
    PIDInstance pid_follow; // 跟随PID

    // 控制命令与状态
    Chassis_Ctrl_Cmd_s cmd;
    Chassis_Upload_Data_s feedback;

    // 内部计算中间变量
    float target_speed_rf; // 右前轮目标速度
    float target_speed_lb; // 左后轮目标速度
    float target_angle_rf; // 右前舵目标角度
    float target_angle_lb; // 左后舵目标角度

    // 上一次的角度记录 (用于就近转动逻辑)
    float last_angle_rf;
    float last_angle_lb;

} Chassis_HalfSteer_t;

/**
 * @brief 初始化半舵轮底盘对象
 * @param chassis 底盘对象指针
 * @param drive_rf 右前驱动电机指针
 * @param drive_lb 左后驱动电机指针
 * @param steer_rf 右前转向电机指针
 * @param steer_lb 左后转向电机指针
 */
void Chassis_HalfSteer_Init(Chassis_HalfSteer_t *chassis, 
                            LKMotorInstance *drive_rf, LKMotorInstance *drive_lb,
                            DJIMotorInstance *steer_rf, DJIMotorInstance *steer_lb);

/**
 * @brief 底盘控制更新函数，建议在RTOS任务中周期调用
 * @param chassis 底盘对象指针
 * @param cmd 控制命令指针
 */
void Chassis_HalfSteer_Update(Chassis_HalfSteer_t *chassis, const Chassis_Ctrl_Cmd_s *cmd);

#endif // CHASSIS_HALF_STEER_H