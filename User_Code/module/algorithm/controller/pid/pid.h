/**
 ******************************************************************************
 * @file	pid.h
 * @author  Wang Hongxi
 * @version V1.1.3
 * @date    2021/7/3
 * @brief
 ******************************************************************************
 * @attention
 *
 ******************************************************************************
 */
#ifndef _PID_H
#define _PID_H

#include "main.h"
#include "stdint.h"
#include "memory.h"
#include "stdlib.h"
#include "bsp_dwt.h"
#include "arm_math.h"
#include <math.h>

#ifndef abs
#define abs(x) ((x > 0) ? x : -x)
#endif

// PID 优化环节使能标志位
typedef enum
{
    PID_IMPROVE_NONE = 0b0000000000,                 // 无优化 0
    PID_Integral_Limit = 0b0000000001,               // 积分限幅 1
    PID_Derivative_On_Measurement = 0b0000000010,    // 微分先行 2
    PID_Trapezoid_Intergral = 0b0000000100,          // 梯形积分 4
    PID_Proportional_On_Measurement = 0b0000001000,  // 比例先行 8
    PID_OutputFilter = 0b0000010000,                 // 输出滤波 16
    PID_ChangingIntegrationRate = 0b0000100000,      // 变速积分 32
    PID_DerivativeFilter = 0b0001000000,             // 微分滤波 64
    PID_ErrorHandle = 0b0010000000,                  // 错误处理 128
    PID_FeedForward = 0b0100000000,                  // 前馈控制 256
    PID_FFC_SimpleMode = 0b1000000000,              // 简单前馈模式 (与PID_FeedForward组合使用) 512
} PID_Improvement_e;

/* PID 报错类型枚举*/
typedef enum errorType_e
{
    PID_ERROR_NONE = 0x00U,
    PID_MOTOR_BLOCKED_ERROR = 0x01U
} ErrorType_e;

typedef struct
{
    uint64_t ERRORCount;
    ErrorType_e ERRORType;
} PID_ErrorHandler_t;

/* PID结构体 */
typedef struct
{
    //---------------------------------- init config block
    // config parameter
    float Kp;
    float Ki;
    float Kd;
    float MaxOut;
    float DeadBand;

    // improve parameter
    PID_Improvement_e Improve;
    float IntegralLimit; // 积分限幅
    float CoefA; // 变速积分 For Changing Integral
    float CoefB; // 变速积分 ITerm = Err*((A-abs(err)+B)/A)  when B<|err|<A+B
    float Output_LPF_RC; // 输出滤波器 RC = 1/omegac
    float Derivative_LPF_RC; // 微分滤波器系数

    //-----------------------------------
    // Feed Forward Control (FFC) parameters
    float FFC_K;  // 简单前馈增益系数
    float FFC_Kp; // 前馈比例系数 (复杂模式)
    float FFC_Kd; // 前馈微分系数 (复杂模式)
    float FFC_Ka; // 前馈加速度系数 (复杂模式)
    float FFC_LPF_RC; // 前馈低通滤波器系数
    float FFC_Output; // 前馈输出值
    float FFC_Set_History[3]; // 前馈设定值历史 [NOW, LAST, LLAST]

    //-----------------------------------
    // for calculating
    float Measure;
    float Last_Measure;
    float Err;
    float Last_Err;
    float Last_ITerm;

    float Pout;
    float Iout;
    float Dout;
    float ITerm;

    float Output;
    float Last_Output;
    float Last_Dout;

    float Ref;

    uint32_t DWT_CNT;
    float dt;

    PID_ErrorHandler_t ERRORHandler;
} PIDInstance;

/* 用于PID初始化的结构体*/
typedef struct // config parameter
{
    // basic parameter
    float Kp;
    float Ki;
    float Kd;
    float MaxOut; // 输出限幅
    float DeadBand; // 死区

    // improve parameter
    PID_Improvement_e Improve;
    float IntegralLimit; // 积分限幅
    float CoefA; // AB为变速积分参数,变速积分实际上就引入了积分分离
    float CoefB; // ITerm = Err*((A-abs(err)+B)/A)  when B<|err|<A+B
    float Output_LPF_RC; // RC = 1/omegac
    float Derivative_LPF_RC;

    // Feed Forward Control (FFC) parameters
    float FFC_K;  // 简单前馈增益系数
    float FFC_Kp; // 前馈比例系数 (复杂模式)
    float FFC_Kd; // 前馈微分系数 (复杂模式)
    float FFC_Ka; // 前馈加速度系数 (复杂模式)
    float FFC_LPF_RC; // 前馈低通滤波器系数
} PID_Init_Config_s;

/**
 * @brief 初始化PID实例
 * @todo 待修改为统一的PIDRegister风格
 * @param pid    PID实例指针
 * @param config PID初始化配置
 */
void PIDInit(PIDInstance *pid, PID_Init_Config_s *config);

/**
 * @brief 计算PID输出
 *
 * @param pid     PID实例指针
 * @param measure 反馈值
 * @param ref     设定值
 * @return float  PID计算输出
 */
float PIDCalculate(PIDInstance *pid, float measure, float ref);

#endif