#include "ffc.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// 内部使用的辅助函数：重置句柄
static void FFC_ResetInstance(FFC_Handle_t *handle) {
    // 将整个结构体清零，相当于 C++ 的值初始化
    memset(handle, 0, sizeof(FFC_Handle_t));
}

float FFC_FeedForwardCalc(FFC_Handle_t *handle, float target) {
    if (handle == NULL) return 0.0f;

    // 获取时间间隔 dt
    handle->dt = DWT_GetDeltaT(&handle->DWT_CNT);

    // 设置当前目标值
    handle->set[FFC_NOW] = target;

    // 低通滤波处理 (完全保留原代码逻辑)
    // 注意：数学上 transform: x = x * dt/(LPF+dt) + x * LPF/(LPF+dt) 等同于 x = x。
    // 这里保留原代码逻辑以防有特殊 side effect 需求。
    float denominator = handle->configs.FFC_LPF + handle->dt;
    if (denominator != 0.0f) {
        handle->set[FFC_NOW] = handle->set[FFC_NOW] * handle->dt / denominator +
                               handle->set[FFC_NOW] * handle->configs.FFC_LPF / denominator;
    }

    // 前馈计算: Proportional + Derivative + Acceleration
    // result = c1*x + c2*v + c3*a
    float p_term = handle->configs.c1 * handle->set[FFC_NOW];
    float d_term = 0.0f;
    float a_term = 0.0f;

    if (handle->dt > 0.000001f) { // 防止除以零
        d_term = handle->configs.c2 * (handle->set[FFC_NOW] - handle->set[FFC_LAST]) / handle->dt;
        a_term = handle->configs.c3 * (handle->set[FFC_NOW] - 2 * handle->set[FFC_LAST] + handle->set[FFC_LLAST]) / (handle->dt * handle->dt);
    }

    handle->result = p_term + d_term + a_term;

    // 限幅
    handle->result = abs_clip(handle->result, handle->configs.MaxOutput);

    // 更新历史状态
    // 注意：原 C++ 代码中先更新 LAST 再更新 LLAST，这会导致移位寄存器逻辑变为 LLAST = (新的)LAST。
    // 为了保持转换的一致性，这里保留原代码的顺序。
    handle->set[FFC_LAST]  = handle->set[FFC_NOW];
    handle->set[FFC_LLAST] = handle->set[FFC_LAST];

    return handle->result;
}

void FFC_SetConfig(FFC_Handle_t *handle, FFC_Init_Config_s config) {
    if (handle == NULL) return;

    // 使用值初始化清除 ffc_s 对象
    FFC_ResetInstance(handle);

    // 复制配置参数
    handle->configs = config;
}

void FFC_CheckPointer(FFC_Handle_t *handle, float *get) {
    if (handle == NULL) return;
    handle->get_data = get;
}

void FFC_Init(FFC_Handle_t *handle, FFC_Init_Config_s config, float *get) {
    if (handle == NULL) return;

    // 对应 C++: FFCSetConfig(config, get)
    FFC_SetConfig(handle, config);
    if (get != NULL) {
        FFC_CheckPointer(handle, get);
    }
}

float FFC_Handle(FFC_Handle_t *handle, float target) {
    return FFC_FeedForwardCalc(handle, target);
}

void FFC_DebugControl(FFC_Handle_t *handle, float *imu_data, float *data_from_gimbalctrl_yaw) {
    if (handle == NULL || imu_data == NULL || data_from_gimbalctrl_yaw == NULL) return;

    handle->debug.count++;
    handle->debug.total_imu_yaw = *imu_data;

    // 生成正弦波目标值
    handle->debug.target = (float)(sin(handle->debug.count * 0.003) * 1.0);

    // 输出到 gimbal ctrl
    *data_from_gimbalctrl_yaw = handle->debug.target;

    // 逻辑判断
    if (fabsf(*data_from_gimbalctrl_yaw) < 0.01f && handle->debug.if_gimbal_ctrl == 0) {
        handle->debug.if_gimbal_ctrl = 1; // true
        DWT_GetDeltaT(&handle->debug.DWT_CNT);
    }

    if (handle->debug.if_gimbal_ctrl == 1 && fabsf(*imu_data) < 0.01f) {
        handle->debug.if_gimbal_ctrl = 0; // false
        handle->debug.error = DWT_GetDeltaT(&handle->debug.DWT_CNT);
    }
}