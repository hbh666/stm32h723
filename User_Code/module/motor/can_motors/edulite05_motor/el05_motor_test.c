/**
 * @file el05_motor_test.c
 * @brief EL05 关节电机最简测试程序(4 电机首次上电验证)
 *
 * 功能:
 *   1. 注册 4 个 EL05 电机(CAN1,电机 CAN_ID = 1~4)
 *   2. 启动控制任务(1kHz 下发 **零力矩**——上电测试安全,电机不会动)
 *   3. 以 1Hz 打印每个电机的反馈诊断(模式/故障/角度/温度),RTT 查看
 *
 * 使用:
 *   本文件在 module 层自动参与编译。用 #ifdef DEV_DEBUG 保护——
 *   只在"调试模式"下接管框架的 __weak ChassisTask;
 *   切到自己的兵种(如 WHEEL_LEG,DEV_DEBUG=OFF)后本测试自动失效。
 *   用 SEGGER RTT Viewer 看日志,或调试器直接 watch edu_motor_instance。
 *
 * 注意:
 *   电机 CAN_ID 默认为 1~4,若出厂 ID 不是 1~4(手册示例为 0x7F),
 *   请用上位机/通信类型7 修改,4 个电机 ID 必须互不相同。
 */
#include "edulite05_motor.h"
#include "bsp_log.h"
#include "cmsis_os.h"
#include "string.h"

#ifdef DEV_DEBUG

/* ---- 4 个电机的配置(ID = 1~4,CAN1) ---- */
static Motor_Init_Config_s el05_config[4] = {
    {.fdcan_init_config = {.can_handle = &hfdcan1, .tx_id = 1}},
    {.fdcan_init_config = {.can_handle = &hfdcan1, .tx_id = 2}},
    {.fdcan_init_config = {.can_handle = &hfdcan1, .tx_id = 3}},
    {.fdcan_init_config = {.can_handle = &hfdcan1, .tx_id = 4}},
};

static EDUMotorInstance *el05_motor[4] = {NULL};

/* 模式状态(反馈帧 ID bit22~23) */
static const char *mode_name(uint8_t s)
{
    switch (s)
    {
        case 0: return "Reset";
        case 1: return "Cali";
        case 2: return "Motor";
        default: return "?";
    }
}

void ChassisTask(void *argument)
{
    (void) argument;

    /* 1. 注册 4 个电机(每个 Init 会发使能帧) */
    for (int i = 0; i < 4; i++)
    {
        el05_motor[i] = EDUMotorInit(&el05_config[i]);
        if (el05_motor[i] == NULL)
        {
            LOGERROR("[EL05 test] motor %d init failed!", i + 1);
        }
    }

    /* 2. 创建 4 个 1kHz 控制任务(下发零力矩) */
    EDUMotorControlInit();
    LOGINFO("[EL05 test] init done, 4 motors registered, zero-torque control running");

    /* 3. 1Hz 打印诊断: 模式/故障/角度/温度 */
    while (1)
    {
        for (int i = 0; i < 4; i++)
        {
            if (el05_motor[i] != NULL)
            {
                /* 注意: SEGGER_RTT_printf 不支持 %f,浮点量转整数打印:
                 * 位置单位毫弧度(×1000),温度单位 0.1℃(×10) */
                LOGINFO("[EL05] id=%d mode=%s fault=0x%02X pos_mrad=%d temp_01C=%d",
                        el05_motor[i]->measure.id,
                        mode_name(el05_motor[i]->measure.mode_status),
                        (unsigned)el05_motor[i]->measure.fault,
                        (int)(el05_motor[i]->measure.position * 1000.0f),
                        (int)(el05_motor[i]->measure.temperature * 10.0f));
            }
        }
        osDelay(1000);
    }
}

#endif /* DEV_DEBUG */
