/**
 * @file leg_kinematics.h
 * @brief 轮腿并联五杆腿机构 运动学正逆解
 *
 * 由 matlab/forwardKinematics.m、matlab/inverseKinematics.m 迁移。
 *
 * 机构模型(单位 mm):
 *   O(0,0) ──L1── A ──L2──┐
 *                          ├── B(x, y) 末端
 *   D(L5,0) ──L4── C ──L3──┘
 * 输入: α(O 轴驱动角)、β(D 轴驱动角)
 *
 * 约定: 角度一律弧度;长度一律毫米。
 * 正解: (α, β) → 末端 (x, y)
 * 逆解: 末端 (x, y) → (α, β),各有两个解,按 Matlab 原版的范围条件选唯一解。
 */
#ifndef LEG_KINEMATICS_H
#define LEG_KINEMATICS_H

/* 正运动学结果 */
typedef struct
{
    float theta1; // 从动杆 L2 绝对角(选定解), rad
    float x, y;   // 末端坐标(选定解), mm

    /* 调试用: 两个候选解 */
    float theta1_1, x_1, y_1;
    float theta1_2, x_2, y_2;
} Leg_FK_Result_s;

/* 逆运动学结果 */
typedef struct
{
    float alpha, beta; // 驱动角(选定解), rad

    /* 调试用: 两个候选解 */
    float alpha1, alpha2;
    float beta1, beta2;
} Leg_IK_Result_s;

/**
 * @brief 正运动学: 驱动角 → 末端坐标
 * @param alpha O 轴驱动角(rad)
 * @param beta  D 轴驱动角(rad)
 * @param result 结果输出
 */
void LegForwardKinematics(float alpha, float beta, Leg_FK_Result_s *result);

/**
 * @brief 逆运动学: 末端坐标 → 驱动角
 * @param x 末端 X 坐标(mm)
 * @param y 末端 Y 坐标(mm)
 * @param result 结果输出
 */
void LegInverseKinematics(float x, float y, Leg_IK_Result_s *result);

#endif // LEG_KINEMATICS_H
