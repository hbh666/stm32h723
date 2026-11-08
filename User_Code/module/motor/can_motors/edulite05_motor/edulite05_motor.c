//
// Created by Lenovo on 2026/11/7.
//
#include "edulite05_motor.h"
#include "bsp_log.h"
#include "cmsis_os.h"
#include "daemon.h"
#include "general_def.h"
#include "memory.h"
#include "motor_def.h"
#include "stdlib.h"
#include "string.h"
#include "user_lib.h"

static  uint8_t idx;
static EDUMotorInstance *edu_motor_instance[EDU_MOTOR_CNT];//建一个全局数组(4 格),每格存一个"指向电机实例的指针",初始全是 NULL,只有本文件能访问。
static TaskHandle_t edu_task_handle[EDU_MOTOR_CNT];

///* 两个用于将uint值和float值进行映射的函数,在设定发送值和解析反馈值时使用 */
static uint16_t float_to_uint(float x, float x_min, float x_max, uint8_t bits)
{
    float span = x_max - x_min;
    float offset = x_min;
    return (uint16_t)((x - offset) * ((float)((1 << bits) - 1)) / span);
}

static float uint_to_float(int x_int, float x_min, float x_max, int bits)