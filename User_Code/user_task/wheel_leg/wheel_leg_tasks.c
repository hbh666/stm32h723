/**
 * @file wheel_leg_tasks.c
 * @brief 自研轮腿机器人兵种代码 (WHEEL_LEG)
 * @note  本目录由根 CMakeLists.txt 的 WHEEL_LEG 开关控制是否参与编译:
 *        仅当 -DWHEEL_LEG=ON(且 DEV_DEBUG 等其他开关为 OFF)时,
 *        本目录下所有 .c/.cpp 才会被加入构建。
 */

#include "robot_def.h"
#include "cmsis_os.h"

/**
 * @brief 轮腿机器人初始化
 * @note  请在 RTOS 任务中调用,不要使用阻塞延时(可用 osDelay / DWT_Delay)
 */
void WheelLegInit(void)
{
    // TODO: 电机、编码器、控制参数初始化
}

/**
 * @brief 兵种任务覆盖示例
 *
 * 框架 Core/Src/freertos.c 里的 __weak 空壳任务(ChassisTask/GimbalTask/
 * ShootTask/CmdTask 等)可以被本目录下的强定义覆盖,链接时强符号自动生效,
 * 无需修改 freertos.c。
 */
/*
void ChassisTask(void *argument)
{
    (void) argument;
    WheelLegInit();
    for (;;)
    {
        // 轮腿控制循环,建议 1kHz
        osDelay(1);
    }
}
*/
