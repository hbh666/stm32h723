#ifndef TRONONEH7_SCAFFOLD_FFC_H
#define TRONONEH7_SCAFFOLD_FFC_H

#include <stdint.h>
#include "bsp_dwt.h"
// #include "user_lib.h" // 假设 abs_clip 在这里，如果没有请取消下面的宏定义注释

// 如果 abs_clip 未定义，提供一个简单的宏实现
#ifndef abs_clip
#define abs_clip(x, max) ((x) > (max) ? (max) : ((x) < -(max) ? -(max) : (x)))
#endif

// 数组索引宏
#define FFC_NOW   0
#define FFC_LAST  1
#define FFC_LLAST 2

// 配置结构体
typedef struct {
    float c1;
    float c2;
    float c3;
    float FFC_LPF;
    float MaxOutput;
} FFC_Init_Config_s;

// Debug 结构体
typedef struct {
    int count;
    float total_imu_yaw;
    float target;
    int if_gimbal_ctrl; // 0: false, 1: true
    uint32_t DWT_CNT;
    float error;
} FFC_Debug_s;

// FFC 句柄结构体 (包含原 ffc_s 的所有内容)
typedef struct {
    float set[3];   // NOW, LAST, LLAST
    float dt;
    float result;
    uint32_t DWT_CNT;
    float *get_data;
    FFC_Init_Config_s configs;
    FFC_Debug_s debug;
} FFC_Handle_t;

// 函数声明

/**
 * @brief 初始化 FFC
 * @param handle FFC 句柄指针
 * @param config 配置结构体
 * @param get 反馈数据指针 (可选，可为 NULL)
 */
void FFC_Init(FFC_Handle_t *handle, FFC_Init_Config_s config, float *get);

/**
 * @brief 设置配置参数
 */
void FFC_SetConfig(FFC_Handle_t *handle, FFC_Init_Config_s config);

/**
 * @brief 绑定反馈指针
 */
void FFC_CheckPointer(FFC_Handle_t *handle, float *get);

/**
 * @brief 前馈计算核心函数
 * @param handle FFC 句柄指针
 * @param target 目标值
 * @return 计算结果
 */
float FFC_FeedForwardCalc(FFC_Handle_t *handle, float target);

/**
 * @brief 处理函数 (包装器)
 */
float FFC_Handle(FFC_Handle_t *handle, float target);

/**
 * @brief Debug 控制逻辑
 * @param handle FFC 句柄指针
 * @param imu_data IMU 数据指针 (输入/输出)
 * @param data_from_gimbalctrl_yaw 云台控制数据指针 (输入/输出)
 */
void FFC_DebugControl(FFC_Handle_t *handle, float *imu_data, float *data_from_gimbalctrl_yaw);

#endif // TRONONEH7_SCAFFOLD_FFC_H
