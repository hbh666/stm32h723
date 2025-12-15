//
// Created by ASUS on 2025/11/17.
//

#ifndef TRONONEH7_SCAFFOLD_RC_H
#define TRONONEH7_SCAFFOLD_RC_H

#endif //TRONONEH7_SCAFFOLD_RC_H

/**
 * @file remote.h
 * @author TuxMonkey (nqx_2004@qq.com)
 * @brief 遥控器模块
 * @version 0.1
 * @date 2025-01-06
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef REMOTE_H
#define REMOTE_H

//@todo：测试define
#define REMOTE_FS_I6X

#include "stdint.h"
#include "main.h"
#include "usart.h"
#include "key_define.h"
#include "robot_def.h"

// 检查接收值是否出错
#define RC_CH_VALUE_MIN    ((uint16_t)0)
#define RC_CH_VALUE_OFFSET ((uint16_t)1024)
#define RC_CH_VALUE_MAX    ((uint16_t)2048)

/* ----------------------- RC Switch Definition----------------------------- */
#define RC_SW_UP   ((uint16_t)1) // 开关向上时的值
#define RC_SW_MID  ((uint16_t)3) // 开关中间时的值
#define RC_SW_DOWN ((uint16_t)2) // 开关向下时的值
// 三个判断开关状态的宏
#define switch_is_down(s) (s == RC_SW_DOWN)
#define switch_is_mid(s)  (s == RC_SW_MID)
#define switch_is_up(s)   (s == RC_SW_UP)


#ifdef REMOTE_DJI_DT7
typedef struct
{
    struct
    {
        int16_t rocker_l_; // 左水平
        int16_t rocker_l1; // 左竖直
        int16_t rocker_r_; // 右水平
        int16_t rocker_r1; // 右竖直
        int16_t dial; // 侧边拨轮

        uint8_t switch_left; // 左侧开关
        uint8_t switch_right; // 右侧开关
    } rc;

    struct
    {
        int16_t x;
        int16_t y;
        int16_t z;
        uint8_t press_l;
        uint8_t press_r;
    } mouse;

    Key_t key[3]; // 改为位域后的键盘索引,空间减少8倍,速度增加16~倍

    uint8_t key_count[3][16];
} RC_ctrl_t;
#endif // REMOTE_DJI_DT7

// @todo 当前结构体嵌套过深,需要进行优化
#ifdef REMOTE_FS_I6X
typedef struct
{
    struct
    {
        // int16_t rocker_l_; // 左水平
        // int16_t rocker_l1; // 左竖直
        // int16_t rocker_r_; // 右水平
        // int16_t rocker_r1; // 右竖直
        // int16_t dial;      // 侧边拨轮

        // uint8_t switch_left;  // 左侧开关
        // uint8_t switch_right; // 右侧开关

        int16_t ch[10];
    } rc;

    struct
    {
        int16_t x;
        int16_t y;
        int16_t z;
        uint8_t press_l;
        uint8_t press_r;
    } mouse;

    uint8_t sw_a; // SWA
    uint8_t sw_b; // SWB
    uint16_t sw_c; // SWC 3
    uint8_t sw_d; // SWD
    int16_t ch1; // 通道1 (云台yaw)
    int16_t ch2; // 通道2 (云台pitch)
    int16_t ch3; // 通道3 (前后)
    int16_t ch4; // 通道4 (左右)
    uint8_t sw_a_last;
    uint8_t sw_b_last;
    uint16_t sw_c_last;
    uint8_t sw_d_last;

    uint8_t sw_a_up_to_down_flag;

    uint8_t sw_b_midtoup_flag;
    uint8_t sw_b_uptomid_flag;
    uint8_t sw_b_midtodown_flag;
    uint8_t sw_b_downtomid_flag;

    uint8_t sw_c_midtoup_flag;
    uint8_t sw_c_uptomid_flag;
    uint8_t sw_c_midtodown_flag;
    uint8_t sw_c_downtomid_flag;
    uint8_t sw_d_up_to_down_flag;
    uint16_t online;

    Key_t key[3]; // 改为位域后的键盘索引,空间减少8倍,速度增加16~倍

    uint8_t key_count[3][16];
} RC_ctrl_t;
#endif // FSI6X
/* ------------------------- Internal Data ----------------------------------- */

/**
 * @brief 初始化遥控器,该函数会将遥控器注册到串口
 *
 * @attention 注意分配正确的串口硬件,遥控器在C板上使用USART3
 *
 */
RC_ctrl_t *RemoteControlInit(UART_HandleTypeDef *rc_usart_handle);

/**
 * @brief 检查遥控器是否在线,若尚未初始化也视为离线
 *
 * @return uint8_t 1:在线 0:离线
 */
uint8_t RemoteControlIsOnline();

#endif // REMOTE_H
