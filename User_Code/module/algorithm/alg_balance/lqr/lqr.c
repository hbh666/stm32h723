/**
 * @file lqr.c
 * @brief 轮式倒立摆平衡 LQR 控制律实现
 */
#include "lqr.h"

float LqrUpdate(const Lqr_Config_s *cfg, float pitch, float pitch_d, float pos, float vel)
{
    /* u = -(K_pitch·θ + K_pitch_d·θ̇ + K_pos·x + K_vel·ẋ) */
    float u = -(cfg->K_pitch * pitch +
                cfg->K_pitch_d * pitch_d +
                cfg->K_pos * pos +
                cfg->K_vel * vel);

    /* 输出限幅,防止异常状态下冲坏电机 */
    if (u > cfg->MaxOutput)
        u = cfg->MaxOutput;
    else if (u < -cfg->MaxOutput)
        u = -cfg->MaxOutput;

    return u;
}
