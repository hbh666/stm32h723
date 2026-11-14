/**
 * @file leg_kinematics.c
 * @brief 轮腿并联五杆腿机构 运动学正逆解(C 迁移版)
 *
 * 来源: matlab/forwardKinematics.m / matlab/inverseKinematics.m
 * 迁移约定:
 *   - Matlab 的 deg2rad 步骤省略,固件全程弧度;
 *   - Matlab 的 mod(angle, 2*pi) 用 user_lib 的 loop_float_constrain 替代;
 *   - 解的选择逻辑与 Matlab 原版保持一致(迁移不改行为)。
 */
#include "leg_kinematics.h"
#include "user_lib.h"     // msin / mcos / PI / loop_float_constrain
#include <math.h>         // sqrtf / atan2f

/* 杆长参数(mm),标定后如需微调改这里 */
#define LEG_L1 150.0f // O→A 驱动杆
#define LEG_L2 250.0f // A→B 从动杆
#define LEG_L3 250.0f // C→B 从动杆
#define LEG_L4 150.0f // D→C 驱动杆
#define LEG_L5 108.0f // O→D 驱动轴间距

/* 等价于 Matlab 的 mod(angle, 2*pi): 归一化到 [0, 2π) */
static float wrap_2pi(float angle)
{
    return loop_float_constrain(angle, 0.0f, 2.0f * PI);
}

void LegForwardKinematics(float alpha, float beta, Leg_FK_Result_s *result)
{
    /* A、C 点坐标 */
    float xa = LEG_L1 * mcos(alpha);
    float ya = LEG_L1 * msin(alpha);
    float xc = LEG_L5 + LEG_L4 * mcos(beta);
    float yc = LEG_L4 * msin(beta);

    /* 三角几何项 */
    float a = 2.0f * (xa - xc) * LEG_L2;
    float b = 2.0f * (ya - yc) * LEG_L2;
    float lac = sqrtf((xa - xc) * (xa - xc) + (ya - yc) * (ya - yc));
    float c = LEG_L3 * LEG_L3 - LEG_L2 * LEG_L2 - lac * lac;

    /* θ1 两个候选解 */
    float theta1_1 = 2.0f * atan2f(b + sqrtf(a * a + b * b - c * c), a + c);
    float theta1_2 = 2.0f * atan2f(b - sqrtf(a * a + b * b - c * c), a + c);

    theta1_1 = wrap_2pi(theta1_1);
    theta1_2 = wrap_2pi(theta1_2);

    /* 解的选择(与 Matlab 原版一致) */
    float theta1 = (theta1_1 < PI / 2.0f) ? theta1_1 : theta1_2;

    result->theta1 = theta1;
    result->x = LEG_L1 * mcos(alpha) + LEG_L2 * mcos(theta1);
    result->y = LEG_L1 * msin(alpha) + LEG_L2 * msin(theta1);

    /* 调试用候选解 */
    result->theta1_1 = theta1_1;
    result->x_1 = LEG_L1 * mcos(alpha) + LEG_L2 * mcos(theta1_1);
    result->y_1 = LEG_L1 * msin(alpha) + LEG_L2 * msin(theta1_1);
    result->theta1_2 = theta1_2;
    result->x_2 = LEG_L1 * mcos(alpha) + LEG_L2 * mcos(theta1_2);
    result->y_2 = LEG_L1 * msin(alpha) + LEG_L2 * msin(theta1_2);
}

void LegInverseKinematics(float x, float y, Leg_IK_Result_s *result)
{
    /* 三角几何项 */
    float a = 2.0f * x * LEG_L1;
    float b = 2.0f * y * LEG_L1;
    float c = x * x + y * y + LEG_L1 * LEG_L1 - LEG_L2 * LEG_L2;

    float d = 2.0f * LEG_L4 * (x - LEG_L5);
    float e = 2.0f * LEG_L4 * y;
    float f = (x - LEG_L5) * (x - LEG_L5) + LEG_L4 * LEG_L4 + y * y - LEG_L3 * LEG_L3;

    /* α、β 各两个候选解 */
    float alpha1 = 2.0f * atan2f(b + sqrtf(a * a + b * b - c * c), a + c);
    float alpha2 = 2.0f * atan2f(b - sqrtf(a * a + b * b - c * c), a + c);
    float beta1 = 2.0f * atan2f(e + sqrtf(d * d + e * e - f * f), d + f);
    float beta2 = 2.0f * atan2f(e - sqrtf(d * d + e * e - f * f), d + f);

    alpha1 = wrap_2pi(alpha1);
    alpha2 = wrap_2pi(alpha2);
    beta1 = wrap_2pi(beta1);
    beta2 = wrap_2pi(beta2);

    /* 解的选择(与 Matlab 原版一致) */
    float alpha = (alpha1 >= PI / 4.0f) ? alpha1 : alpha2;
    float beta = (beta1 <= PI / 4.0f) ? beta1 : beta2;

    result->alpha = alpha;
    result->beta = beta;

    /* 调试用候选解 */
    result->alpha1 = alpha1;
    result->alpha2 = alpha2;
    result->beta1 = beta1;
    result->beta2 = beta2;
}
