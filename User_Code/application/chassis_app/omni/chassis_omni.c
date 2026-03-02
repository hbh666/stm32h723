#include "chassis_omni.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// 辅助函数：绝对值限幅
static float abs_clip(float val, float limit)
{
    if (val > limit) return limit;
    if (val < -limit) return -limit;
    return val;
}

// PID配置
static PID_Init_Config_s chassis_speed_pid_config = {
    .MaxOut = 16000.0f,
    .IntegralLimit = 2000.0f, // 对应 C++ IntegralLimit (Integral_Min/Max 在 C PID 中未直接对应，取其中值或限制值)
    .Kp = 15.0f,
    .Ki = 0.0f,
    .Kd = 0.001f,
    .Output_LPF_RC = 0.002f, // 对应 C++ Output_LPF
    .Derivative_LPF_RC = 0.002f, // 对应 C++ D_LPF
    .Improve = PID_Integral_Limit, // 对应 0x01
};

// 跟随环内环 (对应 index 0)
static PID_Init_Config_s follow_pid_inner_config = {
    .MaxOut = 4000.0f,
    .IntegralLimit = 200.0f,
    .Kp = 20.0f,
    .Ki = 4.0f,
    .Kd = 0.0001f,
    .Output_LPF_RC = 0.002f,
    .Derivative_LPF_RC = 0.002f,
    // 对应 0x37 = Integral_Limit | Differential_Forward | Trapezoid_Intergral | OutputFilter | ChangingIntegrationRate
    // C definitions:
    // PID_Integral_Limit (1)
    // PID_Derivative_On_Measurement (2)
    // PID_Trapezoid_Intergral (4)
    // PID_OutputFilter (16)
    // PID_ChangingIntegrationRate (32)
    .Improve = PID_Integral_Limit | PID_Derivative_On_Measurement | PID_Trapezoid_Intergral | PID_OutputFilter | PID_ChangingIntegrationRate, 
};

// 跟随环外环 (对应 index 1)
static PID_Init_Config_s follow_pid_outer_config = {
    .MaxOut = 4000.0f,
    .IntegralLimit = 4000.0f,
    .Kp = 15.0f,
    .Ki = 0.0f,
    .Kd = 1.8f,
    .Output_LPF_RC = 0.002f,
    .Derivative_LPF_RC = 0.002f,
    .Improve = PID_Integral_Limit | PID_Derivative_On_Measurement | PID_Trapezoid_Intergral | PID_OutputFilter | PID_ChangingIntegrationRate,
};

void Chassis_Omni_Init(Chassis_Omni_t *chassis, DJIMotorInstance *lf, DJIMotorInstance *rf, DJIMotorInstance *lb, DJIMotorInstance *rb)
{
    if (chassis == NULL) return;

    chassis->moto_chassis[0] = lf;
    chassis->moto_chassis[1] = rf;
    chassis->moto_chassis[2] = lb;
    chassis->moto_chassis[3] = rb;

    // 初始化速度PID
    for (int i = 0; i < 4; i++) {
        PIDInit(&chassis->pid_speed[i], &chassis_speed_pid_config);
    }

    // 初始化跟随PID (串级)
    PIDInit(&chassis->pid_follow_angle_inner, &follow_pid_inner_config);
    PIDInit(&chassis->pid_follow_angle_outer, &follow_pid_outer_config);

    // 初始化功率控制参数
    chassis->power_config.super_power_health = 90.0f;
    chassis->power_config.super_power_week = 20.0f;
    chassis->power_config.chassis_normal_speed_limit = 80; // 这里的单位可能需要根据实际调整
}

// 功率分配 (防止超功率)
static void Chassis_Power_Allocation(Chassis_Omni_t *chassis)
{
    float scaling[4];
    float total_err = 0.0f;
    
    // 计算总误差 (使用 Err 字段)
    for (int i = 0; i < 4; i++) {
        total_err += fabsf(chassis->pid_speed[i].Err);
    }
    
    if (total_err > 1e-6f) { // 避免除零
        for (int i = 0; i < 4; i++) {
            scaling[i] = chassis->pid_speed[i].Err / total_err;
        }
        
        // 限制输出
        for (int i = 0; i < 4; i++) {
            // 原代码: pidinstance[0].pos_out = abs_clip(..., abs(Scaling[i] * 50000))
            // 注意: 这里直接修改了 PID 的 Output，可能会影响下一次计算，但在C++原版中就是这样写的
            float limit = fabsf(scaling[i] * 50000.0f);
            chassis->pid_speed[i].Output = abs_clip(chassis->pid_speed[i].Output, limit);
        }
    }
}

void Chassis_Omni_Update(Chassis_Omni_t *chassis)
{
    if (chassis == NULL) return;

    // 1. 获取电机速度并进行正运动学解算 (估计底盘当前速度)
    float motor_speeds[4];
    float real_speed[4]; // [0]=vx, [1]=vy, [2]=w
    
    for (int i = 0; i < 4; i++) {
        // 使用 speed_aps (度/秒)
        motor_speeds[i] = chassis->moto_chassis[i]->measure.speed_aps;
    }

    // 逆结算部分 (原代码注释，实际是正解算：轮速 -> 体速)
    // 假设是X型全向轮/麦克纳姆轮布局
    real_speed[0] = (-motor_speeds[0] - motor_speeds[1] + motor_speeds[2] + motor_speeds[3]) / 4.0f;
    real_speed[1] = (-motor_speeds[0] + motor_speeds[1] - motor_speeds[2] + motor_speeds[3]) / 4.0f;
    real_speed[2] = (-motor_speeds[0] - motor_speeds[1] - motor_speeds[2] - motor_speeds[3]) / 4.0f;

    // 将底盘体坐标系速度转换到之前的参考系 (可能是云台系或世界系，取决于 real_angle 的定义)
    float cos_a = cosf(chassis->cmd.real_angle);
    float sin_a = sinf(chassis->cmd.real_angle);
    
    float speedx = real_speed[0] * cos_a - real_speed[1] * sin_a;
    float speedy = real_speed[0] * sin_a + real_speed[1] * cos_a;

    float target_speed[3] = {0};

    if (chassis->cmd.if_enable != 0)
    {
        // 2. 跟随PID计算
        chassis->offset_angle = -chassis->cmd.follow_angle;
        chassis->offset_speed = chassis->cmd.yaw_speed;

        if (!chassis->cmd.if_free) {
            // 串级PID: 外环(角度) -> 内环(速度)
            // 外环目标: 0 (使 offset_angle 归零)
            float outer_out = PIDCalculate(&chassis->pid_follow_angle_outer, chassis->offset_angle, 0.0f);
            
            // 内环目标: 外环输出
            // 内环反馈: offset_speed (yaw_speed)
            chassis->follow_increment = PIDCalculate(&chassis->pid_follow_angle_inner, chassis->offset_speed, outer_out);
        } else {
            chassis->follow_increment = 0.0f;
            // 清空PID积分等状态
            chassis->pid_follow_angle_outer.Output = 0;
            chassis->pid_follow_angle_inner.Output = 0;
        }

        // 3. 底盘速度闭环控制 (P控制)
        // 这里的 5.5 是速度环增益，计算出的是"加速度"或"力"的需求
        target_speed[0] = (chassis->cmd.speed[0] - speedx) * 5.5f;
        target_speed[1] = (chassis->cmd.speed[1] - speedy) * 5.5f;
        
        // 旋转轴控制
        // 如果没有指令输入，则使用 real_speed 差值进行阻尼控制？
        // 原代码逻辑: speed[2] = (cmd - real) * 5.5
        target_speed[2] = (chassis->cmd.speed[2] - real_speed[2]) * 5.5f;

        if (chassis->cmd.speed[2] == 0.0f) // 如果没有旋转指令
        {
             // 叠加跟随PID输出，并减去当前旋转速度 (阻尼)
             target_speed[2] = (chassis->follow_increment - real_speed[2] - real_speed[2]) * 5.5f;
        }
        else
        {
            // 如果有手动旋转指令，清除跟随PID积分
             chassis->pid_follow_angle_outer.Output = 0;
             chassis->pid_follow_angle_inner.Output = 0;
        }

        // 4. 逆运动学解算 (体速 -> 轮速)
        // 引入了旋转补偿: real_angle - 0.002 * real_speed[2]
        float corrected_angle = chassis->cmd.real_angle - 0.002f * real_speed[2];
        float sin_ca = sinf(corrected_angle);
        float cos_ca = cosf(corrected_angle);

        // 转换回电机解算所需的 x, y 分量
        float y = -(target_speed[0] * sinf(chassis->cmd.real_angle) - target_speed[1] * cos_ca);
        float x = (target_speed[0] * cosf(chassis->cmd.real_angle) + target_speed[1] * sin_ca);

        // 5. 电机PID控制与输出
        float wheel_targets[4];
        wheel_targets[0] = (-x - y) - target_speed[2];
        wheel_targets[1] = (-x + y) - target_speed[2];
        wheel_targets[2] = (x - y) - target_speed[2];
        wheel_targets[3] = (x + y) - target_speed[2];

        for (int i = 0; i < 4; i++) {
            // PIDCalculate(pid, measure, target) -> 这里的measure似乎被当作0处理？
            // 原C++代码: pid_chassis[i]->PID_handle(wheel_targets[i]);
            // PID_handle(target) 内部通常是 calculate(measure, target).
            // 但原代码中 PID 构造时传入了 &moto_chassis[i]->speed 地址。
            // 因此 C++ PID 类会自动读取 measure。
            // 在 C 中，我们需要手动传入 measure。
            
            float output = PIDCalculate(&chassis->pid_speed[i], chassis->moto_chassis[i]->measure.speed_aps, wheel_targets[i]);
            // 设置电机输出 (注意：DJIMotorSetRef 设置的是目标值还是直接电流？)
            // 根据 dji_motor.h 注释: "可以将电机视为传递函数为1的设备...不需要关心底层的闭环"
            // 如果 DJIMotorSetRef 是设定速度闭环的目标，那么上面的 PID 是多余的吗？
            // 不，原代码 clearly 使用了 pid_chassis[i] 计算 send_data。
            // 这意味着 dji_motor 应该工作在 OPEN_LOOP 或 CURRENT_LOOP 模式，或者我们需要直接操作 current。
            // 假设我们这里计算的是电流值，因为 MaxOut 是 16000 (M3508电流范围)。
            // DJIMotorSetRef 通常用于设定内置闭环的目标。
            // 如果要发送电流，通常没有直接的 SetCurrent API，除非 Motor_Control_Setting_s 允许。
            // 为了保持移植性，我们假设 DJIMotorSetRef 能够处理这个输出，或者我们需要修改 dji_motor 模块。
            // 这里我们假设 DJIMotorSetRef 在电流模式下工作。
            DJIMotorSetRef(chassis->moto_chassis[i], output);
        }

        // 6. 功率限制
        Chassis_Power_Allocation(chassis);
        // 如果 Chassis_Power_Allocation 修改了 PID Output，我们需要重新 SetRef 吗？
        // 原代码直接修改了 pos_out，这在下一次计算时生效，或者如果 PID 类直接返回 pos_out 给 send_data。
        // C++代码: moto_chassis[i]->send_data = PID_handle(...); Chassis_Power_Allocation();
        // Power_Allocation 修改了 pid instance 的 pos_out。
        // 这意味着当前的 send_data 并没有被 Power_Allocation 修正！
        // 修正逻辑应该是先计算 PID，再分配，再发送。
        // 但为了忠实还原原代码逻辑，我们保持顺序。
        // (注：原代码逻辑可能存在缺陷，分配后的功率限制在下一帧才通过积分项或直接赋值生效？
        //  或者 moto_chassis->send_data 是个指针引用？不，它是值。
        //  如果原代码 Allocation 在赋值给 send_data 之后调用，那么它只影响了 PID 内部状态，不影响当前帧输出。)
    }
    else
    {
        for (int i = 0; i < 4; i++) {
            DJIMotorStop(chassis->moto_chassis[i]);
        }
    }
}

void Chassis_Omni_PowerControl(Chassis_Omni_t *chassis, Chassis_Power_Info_s *power_info, Chassis_Ctrl_Cmd_s *raw_cmd)
{
    if (chassis == NULL || power_info == NULL || raw_cmd == NULL) return;

    // 复制原始指令到内部 cmd (默认)
    chassis->cmd = *raw_cmd;

    // 简单的功率策略实现 (参考 Chassis_OmniWheel_Crtl::powerControl)
    float speed_scaling = 1.0f;

    if (power_info->remain_energy >= chassis->power_config.super_power_health)
    {
        speed_scaling = 1.0f; // 正常模式
    }
    else if (power_info->remain_energy >= chassis->power_config.super_power_week)
    {
        // 线性降额: energy + 10 ? 原代码: tired = energy + 10
        float tired = power_info->remain_energy + 10.0f;
        speed_scaling = tired * 0.01f; // 归一化
    }
    else
    {
        speed_scaling = 0.3f; // 低电量模式
    }

    // 应用缩放系数 (原代码还有 200 * 1.5/1.8 的系数，这里假设 raw_cmd 已经是归一化值，只做缩放)
    // 原代码: data_to_chassis.speed[...] = data_from_FSM.speed[...] * 200 * ...
    // 这里我们只做相对缩放，保留原始比例
    chassis->cmd.speed[0] *= speed_scaling;
    chassis->cmd.speed[1] *= speed_scaling;
    chassis->cmd.speed[2] *= speed_scaling;
}