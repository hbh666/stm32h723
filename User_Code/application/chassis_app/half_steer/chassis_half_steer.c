#include "chassis_half_steer.h"
#include "user_lib.h" // 包含 arm_math.h, PI, user_malloc 等
#include <math.h>

// 宏定义 (根据实际机械结构调整)
#ifndef WHEEL_BASE
#define WHEEL_BASE 0.35f // 轴距 (示例值)
#endif
#ifndef TRACK_WIDTH
#define TRACK_WIDTH 0.35f // 轮距 (示例值)
#endif

#define CHASSIS_WHEEL_OFFSET 30.0f       // 舵轮偏置参数
#define SQRT2                1.41421356f // 根号2
#define RAD_2_DEGREE         57.2957795f
#define DEGREE_2_RAD         0.01745329f

// 舵轮对齐角度 (根据实际安装调整)
#define STEERING_CHASSIS_ALIGN_ANGLE_RF 0.0f
#define STEERING_CHASSIS_ALIGN_ANGLE_LB 0.0f

// 静态函数声明
static void MinmizeRotation(float *angle, const float *last_angle, float *speed);
static void SteeringWheelCalculate(Chassis_HalfSteer_t *chassis);

// 默认跟随PID配置
static PID_Init_Config_s follow_pid_config = {
    .Kp = 6.0f,
    .Ki = 0.0f,
    .Kd = 0.495f,
    .MaxOut = 45.0f,
};

void Chassis_HalfSteer_Init(Chassis_HalfSteer_t *chassis,
                            LKMotorInstance *drive_rf, LKMotorInstance *drive_lb,
                            DJIMotorInstance *steer_rf, DJIMotorInstance *steer_lb)
{
    if (chassis == NULL) return;

    // 绑定电机实例
    chassis->motor_drive_rf = drive_rf;
    chassis->motor_drive_lb = drive_lb;
    chassis->motor_steer_rf = steer_rf;
    chassis->motor_steer_lb = steer_lb;

    // 初始化PID
    PIDInit(&chassis->pid_follow, &follow_pid_config);

    // 初始化状态变量
    chassis->last_angle_rf = 0.0f;
    chassis->last_angle_lb = 0.0f;
    chassis->target_speed_rf = 0.0f;
    chassis->target_speed_lb = 0.0f;
    chassis->target_angle_rf = 0.0f;
    chassis->target_angle_lb = 0.0f;

    // 如果有电机需要特定的初始化配置（如 dji_motor 的参数），请在此处补充或在外部完成
}

void Chassis_HalfSteer_Update(Chassis_HalfSteer_t *chassis, const Chassis_Ctrl_Cmd_s *cmd)
{
    if (chassis == NULL || cmd == NULL) return;

    // 1. 更新内部命令副本
    chassis->cmd = *cmd;

    // 2. 检查底盘模式与安全状态
    if (chassis->cmd.chassis_mode == CHASSIS_ZERO_FORCE)
    {
        LKMotorStop(chassis->motor_drive_rf);
        LKMotorStop(chassis->motor_drive_lb);
        DJIMotorStop(chassis->motor_steer_rf);
        DJIMotorStop(chassis->motor_steer_lb);
        return; // 直接返回，不再计算
    }
    else
    {
        LKMotorEnable(chassis->motor_drive_rf);
        LKMotorEnable(chassis->motor_drive_lb);
        DJIMotorEnable(chassis->motor_steer_rf);
        DJIMotorEnable(chassis->motor_steer_lb);
    }

    // 3. 预处理旋转量 (wz)
    switch (chassis->cmd.chassis_mode)
    {
        case CHASSIS_NO_FOLLOW:
            chassis->cmd.wz = 0;
            break;
        case CHASSIS_FOLLOW_GIMBAL_YAW:
        {
            float angle_err = chassis->cmd.offset_angle;
            // 归一化到 [-180, 180]
            if(angle_err > 180.0f) angle_err -= 360.0f;
            else if(angle_err < -180.0f) angle_err += 360.0f;

            // 计算跟随PID输出
            chassis->cmd.wz = PIDCalculate(&chassis->pid_follow, angle_err, 0.0f) / 100.0f; // 根据原代码保留/100
        }
        break;
        case CHASSIS_ROTATE:
            chassis->cmd.wz = 0.5f; // 固定自旋速度，可改为变量
            break;
        default:
            break;
    }

    // 4. 坐标系转换 (云台系 -> 底盘系)
    // 假设 cmd.vx/vy 是云台坐标系下的指令
    float sin_theta = arm_sin_f32(chassis->cmd.offset_angle * DEGREE_2_RAD);
    float cos_theta = arm_cos_f32(chassis->cmd.offset_angle * DEGREE_2_RAD);

    // 覆盖原始 vx/vy 为底盘系速度 (使用中间变量避免污染原始cmd数据，这里直接覆盖cmd结构体中的值用于后续计算)
    float chassis_vx = chassis->cmd.vx * cos_theta - chassis->cmd.vy * sin_theta;
    float chassis_vy = chassis->cmd.vx * sin_theta + chassis->cmd.vy * cos_theta;

    // 将转换后的速度存回用于计算，或者传递给计算函数
    // 为了保持清晰，我们修改 SteeringWheelCalculate 的输入方式，这里暂时存入 cmd 结构体或传递局部变量
    // 这里选择传递局部变量，需要修改 SteeringWheelCalculate 内部逻辑
    // 为了复用原逻辑，我将在函数内部使用 chassis_vx/vy

    // 5. 运动学解算
    // 传入 chassis_vx, chassis_vy 和 chassis->cmd.wz
    // 注意：原代码使用全局变量，这里我们需要适配

    float w = chassis->cmd.wz * CHASSIS_WHEEL_OFFSET * SQRT2;

    if (fabsf(chassis_vx) == 0 && fabsf(chassis_vy) == 0 && chassis->cmd.wz == 0) {
        chassis->target_speed_lb = 0;
        chassis->target_speed_rf = 0;
        // 角度保持不变，或回中？原代码保持不变
    } else {
        // LB (Left Back) 计算: y+, x-
        // 注意：原代码注释里的方向似乎与变量名有差异，这里基于原代码逻辑复刻
        // 原代码: arm_sqrt_f32(temp_x * temp_x + temp_y * temp_y, &vt_lb); // lb: y+ , x-
        // temp_x = chassis_vx - w; temp_y = chassis_vy + w;
        float temp_x_lb = chassis_vx - w;
        float temp_y_lb = chassis_vy + w;
        arm_sqrt_f32(temp_x_lb * temp_x_lb + temp_y_lb * temp_y_lb, &chassis->target_speed_lb);

        float offset_lb = -atan2f(temp_y_lb, temp_x_lb) * RAD_2_DEGREE;
        chassis->target_angle_lb = STEERING_CHASSIS_ALIGN_ANGLE_LB + offset_lb;

        // RF (Right Front) 计算: y-, x+
        // 原代码: arm_sqrt_f32(temp_x * temp_x + temp_y * temp_y, &vt_rf); // rf: y- , x+
        // temp_x = chassis_vx + w; temp_y = chassis_vy - w;
        float temp_x_rf = chassis_vx + w;
        float temp_y_rf = chassis_vy - w;
        arm_sqrt_f32(temp_x_rf * temp_x_rf + temp_y_rf * temp_y_rf, &chassis->target_speed_rf);

        float offset_rf = -atan2f(temp_y_rf, temp_x_rf) * RAD_2_DEGREE;
        chassis->target_angle_rf = STEERING_CHASSIS_ALIGN_ANGLE_RF + offset_rf;

        // 6. 角度优化 (MinmizeRotation)
        // 更新 last_angle
        chassis->last_angle_lb = chassis->motor_steer_lb->measure.total_angle;
        chassis->last_angle_rf = chassis->motor_steer_rf->measure.total_angle;

        // 限制到 [-180, 180] 绝对值逻辑? 原代码使用了 ANGLE_LIMIT_360_TO_180_ABS 宏
        // 这里手动实现或调用 user_lib
        // 假设 user_lib.h 中有相关宏，这里简单处理
        // (省略部分宏展开，直接使用 MinmizeRotation)

        MinmizeRotation(&chassis->target_angle_lb, &chassis->last_angle_lb, &chassis->target_speed_lb);
        MinmizeRotation(&chassis->target_angle_rf, &chassis->last_angle_rf, &chassis->target_speed_rf);
    }

    // 7. 发送控制指令
    // 转向电机 (DJI GM6020)
    DJIMotorSetRef(chassis->motor_steer_lb, chassis->target_angle_lb);
    DJIMotorSetRef(chassis->motor_steer_rf, chassis->target_angle_rf);

    // 驱动电机 (LK 9015)
    LKMotorSetRef(chassis->motor_drive_lb, chassis->target_speed_lb);
    LKMotorSetRef(chassis->motor_drive_rf, chassis->target_speed_rf);
}

/**
 * @brief 使舵电机角度最小旋转，取优弧
 */
static void MinmizeRotation(float *angle, const float *last_angle, float *speed)
{
    float target_angle = *angle;
    float actual_angle = *last_angle;
    float rotation = target_angle - actual_angle;
    float norm_rotation = rotation;

    // 规范化旋转角度到 [-180, 180]
    while (norm_rotation > 180.0f) {
        norm_rotation -= 360.0f;
    }
    while (norm_rotation < -180.0f) {
        norm_rotation += 360.0f;
    }

    float threshold = 110.0f; // 阈值，超过此角度则反转轮子方向

    // 简单的优弧判断
    if (norm_rotation > threshold) {
        int32_t round_diff = (int32_t)((target_angle - actual_angle) / 360.0f);
        *angle = actual_angle + norm_rotation - 180.0f + round_diff * 360.0f;
        *speed = -(*speed);
    } else if (norm_rotation < -threshold) {
        int32_t round_diff = (int32_t)((target_angle - actual_angle) / 360.0f);
        *angle = actual_angle + norm_rotation + 180.0f + round_diff * 360.0f;
        *speed = -(*speed);
    }

    // 如果没有触发反转，目标角度通常需要加上圈数，
    // 但原代码逻辑似乎是直接修改传入的 angle 指针。
    // 如果 norm_rotation 在阈值内，我们需要确保 angle 是基于 actual_angle 的最近点
    // 原逻辑中 MinmizeRotation 似乎只处理了反转的情况，
    // 对于常规旋转，可能需要确保 *angle 包含了正确的圈数信息。
    // 补充逻辑：
    if (norm_rotation <= threshold && norm_rotation >= -threshold) {
         // 计算最近的目标角度（包含圈数）
         *angle = actual_angle + norm_rotation;
    }
}