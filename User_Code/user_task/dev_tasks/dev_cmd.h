//
// Created by ASUS on 2025/12/15.
//

#ifndef TRONONEH7_SCAFFOLD_DEV_CMD_H
#define TRONONEH7_SCAFFOLD_DEV_CMD_H

// 如果用到了 FreeRTOS 的 API，可以在这里包含，或者在需要用到的源文件里包含
// #include "cmsis_os2.h"

/**
 * @brief 机器人指令控制及外设初始化
 */
void RobotCMDInit(void);

/**
 * @brief 指令下发主任务 (FreeRTOS 线程)
 * @param argument 线程传入参数
 */
void CmdTask(void *argument);

#endif // TRONONEH7_SCAFFOLD_DEV_CMD_H
