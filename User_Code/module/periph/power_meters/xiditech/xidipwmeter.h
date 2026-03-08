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
} XidiPowerMeter_Msg_s;

/* 功率计实例结构 */
typedef struct
{
    FDCANInstance *fdcan_ins; // FDCAN实例指针
    XidiPowerMeter_Msg_s powermeter_msg; // 测量数据
    Daemon_Instance *daemon; // 守护进程
    uint32_t feed_cnt; // 喂狗计数器
} XidiPowerMeterInstance;

/* 功率计初始化配置 */
typedef struct {
    FDCAN_Init_Config_s can_config;
    Daemon_Init_Config_s daemon_config;
} XidiPowerMeter_Init_Config_s;

/* 函数声明 */
XidiPowerMeterInstance *XidiPowerMeterInit(XidiPowerMeter_Init_Config_s *XidiPowerMeter_config);

void XidiPowerMeterDecode(FDCANInstance *_instance);

void XidiPowerMeterLostCallback(void *power_meter_ptr);

float XidiPowerMeterGetVoltage(XidiPowerMeterInstance *power_meter);

float XidiPowerMeterGetCurrent(XidiPowerMeterInstance *power_meter);

float XidiPowerMeterGetPower(XidiPowerMeterInstance *power_meter);

float XidiPowerMeterGetEnergy(XidiPowerMeterInstance *power_meter);

void XidiPowerMeterResetEnergy(XidiPowerMeterInstance *power_meter);

XidiPowerMeterInstance *GetPowerMeterInstance(void);

#endif //TRONONEH7_SCAFFOLD_XIDIPWMETER_H
