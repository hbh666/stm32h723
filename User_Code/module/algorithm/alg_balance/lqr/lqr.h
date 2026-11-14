/**
 * @file lqr.h
 * @brief 轮式倒立摆平衡 LQR 控制(离线求 K,在线做矩阵乘)
 *
 * 状态: x = [θ, θ̇, x, ẋ]ᵀ = [俯仰角 rad, 俯仰角速度 rad/s, 轮位移 m, 轮速度 m/s]
 * 输出: u = 轮扭矩 Nm,控制律 u = -K·x
 *
 * K 增益由 matlab/wheel_control.m 离线求得:
 *   m=6.25kg(俯仰质量) h=0.2156m(最高点重心) r=0.07m M=3.7kg
 *   Q=diag([1 1 1 1]), R=1  →  K = [-15.2371 -3.4316 -1.0000 -4.6233]
 * 调参方式: 改 Q/R 重新跑 lqr(),把新 K 填入 LQR_DEFAULT_CONFIG。
 */
#ifndef LQR_H
#define LQR_H

typedef struct
{
    float K_pitch;   // θ  俯仰角增益
    float K_pitch_d; // θ̇ 俯仰角速度增益
    float K_pos;     // x  轮位移增益
    float K_vel;     // ẋ  轮速度增益
    float MaxOutput; // 输出限幅(Nm),按轮毂电机扭矩上限设置
} Lqr_Config_s;

/* 默认配置 = matlab/wheel_control.m 当前求解结果(Q=diag([1,1,1,1]), R=1) */
#define LQR_DEFAULT_CONFIG                         \
    {                                              \
        .K_pitch = -15.2371f,                      \
        .K_pitch_d = -3.4316f,                     \
        .K_pos = -1.0000f,                         \
        .K_vel = -4.6233f,                         \
        .MaxOutput = 6.0f, /* TODO: 按实际轮毂电机扭矩上限修改 */ \
    }

/**
 * @brief LQR 平衡控制律: u = -K·x
 * @param cfg   增益配置
 * @param pitch     俯仰角 θ(rad,需零位标定: 站立平衡点 = 0)
 * @param pitch_d   俯仰角速度 θ̇(rad/s)
 * @param pos       轮位移 x(m)
 * @param vel       轮速度 ẋ(m/s)
 * @return 轮扭矩指令 u(Nm)
 */
float LqrUpdate(const Lqr_Config_s *cfg, float pitch, float pitch_d, float pos, float vel);

#endif // LQR_H
