/**
* @file robot.c
 * @brief
 * @author TuxMonkey (nqx2004@gmail.com)
 * @version 1.0
 * @date 2025-07-08
 *
 * @copyright Copyright (c) 2025  DLMU-C.ONE
 *
 * @par 修改日志:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2025-07-08 <td>1.0     <td>TuxMonkey     <td>内容
 * </table>
 */

#include "bsp_init.h"
#include "robot.h"

#include "cmsis_gcc.h"
// #include "robot_def.h"
// #include "robot_task.h"

// // 编译warning,提醒开发者修改机器人参数
// #ifndef ROBOT_DEF_PARAM_WARNING
// #define ROBOT_DEF_PARAM_WARNING
// #pragma message "check if you have configured the parameters in robot_def.h, IF NOT, please refer to the comments AND DO IT, otherwise the robot will have FATAL ERRORS!!!"
// #endif // !ROBOT_DEF_PARAM_WARNING

// #if defined(ONE_BOARD) || defined(CHASSIS_BOARD)
// #include "chassis.h"
// #endif

// #if defined(ONE_BOARD) || defined(GIMBAL_BOARD)
// #include "gimbal.h"
// #include "shoot.h"
// #include "robot_cmd.h"
// #endif

/**
 * @brief 机器人初始化函数
 *
 * @note 该函数在系统启动时调用,用于初始化机器人各个模块
 *
 * @attention 请不要在初始化过程中使用中断和延时函数！
 * 若必须,则只允许使用DWT_Delay()
 */

void RobotInit()
{
    // 关闭中断,防止在初始化过程中发生中断
    // 请不要在初始化过程中使用中断和延时函数！
    // 若必须,则只允许使用DWT_Delay()
    __disable_irq();

    BSPInit();

#if defined(ONE_BOARD) || defined(GIMBAL_BOARD)
    RobotCMDInit();
    // GimbalInit();
    // ShootInit();
#endif

#if defined(ONE_BOARD) || defined(CHASSIS_BOARD)
    ChassisInit();
#endif

    // OSTaskInit(); // 创建基础任务

    // 初始化完成,开启中断
    __enable_irq();
}

// void RobotTask()
// {
// #if defined(ONE_BOARD) || defined(GIMBAL_BOARD)
//     RobotCMDTask();
//     // GimbalTask();
//     // ShootTask();
// #endif

// #if defined(ONE_BOARD) || defined(CHASSIS_BOARD)
//     ChassisTask();
// #endif

// }
