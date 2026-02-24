//
// Created by ASUS on 2025/11/17.
//

#ifndef TRONONEH7_SCAFFOLD_XIDIPWMETER_H
#define TRONONEH7_SCAFFOLD_XIDIPWMETER_H

#include "bsp_dwt.h"
#include "bsp_fdcan.h"
#include "daemon.h"
#include "general_def.h"

/* 功率计测量数据结构 */
typedef struct
{
    float voltage; // 电压 (V)
    float current; // 电流 (A)
    float power; // 功率 (W)
    float energy; // 累计能量 (J)
    uint32_t update_cnt; // 更新计数器
    float dt; // 更新时间间隔
} PowerMeter_Measure_s;

/* 功率计实例结构 */
typedef struct
{
    FDCANInstance *can_instance; // FDCAN实例指针
    PowerMeter_Measure_s measure; // 测量数据
    Daemon_Instance *daemon; // 守护进程
    uint32_t feed_cnt; // 喂狗计数器
} PowerMeterInstance;

/* 函数声明 */
PowerMeterInstance *PowerMeterInit(FDCAN_HandleTypeDef *can_handle);

void PowerMeterDecode(FDCANInstance *instance);

void PowerMeterLostCallback(void *power_meter_ptr);

float PowerMeterGetVoltage(PowerMeterInstance *power_meter);

float PowerMeterGetCurrent(PowerMeterInstance *power_meter);

float PowerMeterGetPower(PowerMeterInstance *power_meter);

float PowerMeterGetEnergy(PowerMeterInstance *power_meter);

void PowerMeterResetEnergy(PowerMeterInstance *power_meter);

PowerMeterInstance *GetPowerMeterInstance(void);

#endif //TRONONEH7_SCAFFOLD_XIDIPWMETER_H
