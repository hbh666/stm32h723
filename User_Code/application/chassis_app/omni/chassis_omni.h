#ifndef CHASSIS_OMNI_H
#define CHASSIS_OMNI_H

#include "stdint.h"
#include "dji_motor.h"
#include "pid.h"

// 定义底盘控制命令结构体 (由于chassis_ctrl.h为空，在此定义以适配逻辑)
typedef struct
{
    float speed[3];      // x, y, z (旋转) 速度设定值
    float follow_angle;  // 跟随角度 (底盘与云台夹角)
    float yaw_speed;     // 当前Yaw轴角速度 (作为前馈或反馈)
    float real_angle;    // 底盘当前实际角度 (用于坐标系转换)
    uint8_t if_enable;   // 底盘使能标志
    uint8_t if_free;     // 底盘自由模式标志 (不跟随)
} Chassis_Ctrl_Cmd_s;

// 定义功率控制所需的外部数据结构
typedef struct
{
    uint16_t chassis_power_limit; // 来自裁判系统的功率限制
    float remain_energy;          // 来自超级电容的剩余能量
    uint16_t chassis_power_buffer; // 缓冲能量 (可选)
} Chassis_Power_Info_s;

// 全向轮底盘对象结构体
typedef struct
{
    // 电机实例指针 (LF, RF, LB, RB)
    DJIMotorInstance *moto_chassis[4];

    // 速度环PID实例 (每个轮子一个)
    PIDInstance pid_speed[4];

    // 跟随环串级PID实例
    PIDInstance pid_follow_angle_outer; // 外环 (角度)
    PIDInstance pid_follow_angle_inner; // 内环 (角速度)

    // 控制数据
    Chassis_Ctrl_Cmd_s cmd;
    
    // 内部计算状态变量
    float offset_angle;
    float offset_speed;
    float follow_increment;
    
    // 功率控制参数
    struct {
        float super_power_health;
        float super_power_week;
        uint16_t chassis_normal_speed_limit;
    } power_config;

} Chassis_Omni_t;

/**
 * @brief 初始化全向轮底盘对象
 * @param chassis 底盘对象指针
 * @param lf 左前电机指针
 * @param rf 右前电机指针
 * @param lb 左后电机指针
 * @param rb 右后电机指针
 */
void Chassis_Omni_Init(Chassis_Omni_t *chassis, DJIMotorInstance *lf, DJIMotorInstance *rf, DJIMotorInstance *lb, DJIMotorInstance *rb);

/**
 * @brief 底盘控制任务函数，建议在RTOS任务中周期调用
 * @param chassis 底盘对象指针
 */
void Chassis_Omni_Update(Chassis_Omni_t *chassis);

/**
 * @brief 功率控制逻辑，根据裁判系统和超电状态限制目标速度
 * @param chassis 底盘对象指针
 * @param power_info 功率状态信息
 * @param raw_cmd 原始控制命令 (通常来自上层FSM)
 */
void Chassis_Omni_PowerControl(Chassis_Omni_t *chassis, Chassis_Power_Info_s *power_info, Chassis_Ctrl_Cmd_s *raw_cmd);

#endif // CHASSIS_OMNI_H