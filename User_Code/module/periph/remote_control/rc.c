
/*************************发射机DT7***************************
 *                                                            *
 *   -----------------------------------------------------    *
 *   |     (上-1)                               (上-1)   |    *
 *   |SW_L|(中-3)                          SW_R|(中-3)   |    *
 *   |     (下-2)                               (下-2)   |    *
 *   |                                                   |    *
 *   |    | ^ |                                | ^ |     |    *
 *   |    | 3 |左摇杆                     右摇杆| 1 |     |    *
 *   | ---     ---                          ---     ---  |    *
 *   |<           2>                       <           0>|    *
 *   | ---     ---                          ---     ---  |    *
 *   |    |   |                                |   |     |    *
 *   |    |   |                                |   |     |    *
 *   |                                                   |    *
 *   -----------------------------------------------------    *
 *                                                            *
 **************************遥控器信息**************************
 *域            通道0      通道1   通道2   通道3   S1     S2  *
 *偏移            0          11      22      33    44     46  *
 *长度(bit)      11          11      11      11     2      2  *
 *符号位         无          无      无      无    无     无  *
 *范围           ***********最大值1684*********    *最大值3*  *
 *               *          中间值1024        *    *最小值1*  *
 *               ***********最小值364**********               *
 *功能                                            1:上   1:上 *
 *                                                2:下   2:下 *
 *                                                3:中   3:中 *
 *                                                            *
 ***************************鼠标信息***************************
 *域         鼠标x轴   鼠标y轴   鼠标z轴   鼠标左键   鼠标右键*
 *偏移         48        64        80        86        94   *
 *长度         16        16        16        8         8    *
 *符号位       有        有        有        无         无   *
 *范围         ******最大值32767*****        ***最大值1***   *
 *             *     最小值-32768   *        ***最小值0***   *
 *             ******静止值0*********                        *
 *功能       ***鼠标在XYZ轴的移动速度***   *鼠标左右键是否按下*
 *           *  负值表示往左移动       *   *    0:没按下      *
 *           ***正值表示往右移动********   *****1:按下*********
 *                                                           *
 *                                                           *
 ***************************键盘信息***************************
 *域              按键                                        *
 *偏移            102                                        *
 *长度            16                                         *
 *符号位          无                                          *
 *范围            位值标识                                    *
 *功能            每个按键对应一个bit                          *
 *                Bit 0:W键                                   *
 *                Bit 1:S键                                   *
 *                Bit 2:A键                                   *
 *                Bit 3:D键                                   *
 *                Bit 4:Shift键                               *
 *                Bit 5:Ctrl键                                *
 *                Bit 6:Q键                                   *
 *                Bit 7:E键                                   *
 *                Bit 8:R键                                   *
 *                Bit 9:F键                                   *
 *                Bit10:G键                                   *
 *                Bit11:Z键                                   *
 *                Bit12:X键                                   *
 *                Bit13:C键                                   *
 *                Bit14:V键                                   *
 *                Bit15:B键                                   *
 **************************************************************/

#include "rc.h"
#include "string.h"
#include "bsp_usart.h"
#include "memory.h"
#include "stdlib.h"
#include "daemon.h"
#include "indicator_app/ws2812status.h"
#include "robot_def.h"

#define REMOTE_CONTROL_FRAME_SIZE 25u // 遥控器接收的buffer大小

//@todo：测试define
#define REMOTE_FS_I6X

// 定义SBUS协议的起始标志
#define SBUS_HEAD 0X0F

// 定义SBUS协议的结束标志
#define SBUS_END 0X00

// 遥控器数据
static RC_ctrl_t rc_ctrl[2]; //[0]:当前数据TEMP,[1]:上一次的数据LAST.用于按键持续按下和切换的判断
static uint8_t rc_init_flag = 0; // 遥控器初始化标志位

// 遥控器拥有的串口实例,因为遥控器是单例,所以这里只有一个,就不封装了
static USART_Instance *rc_usart_instance;
static Daemon_Instance *rc_daemon_instance;


#ifdef REMOTE_DJI_DT7
/**
 * @brief 矫正遥控器摇杆的值,超过660或者小于-660的值都认为是无效值,置0
 *
 */
static void RectifyRCjoystick()
{
    for (uint8_t i = 0; i < 5; ++i)
        if (abs(*(&rc_ctrl[TEMP].rc.rocker_l_ + i)) > 660)
            *(&rc_ctrl[TEMP].rc.rocker_l_ + i) = 0;
}

/**
 * @brief 遥控器数据解析
 *
 * @param sbus_buf 接收buffer
 */
static void sbus_to_rc(const uint8_t *sbus_buf)
{
    // 摇杆,直接解算时减去偏置
    rc_ctrl[TEMP].rc.rocker_r_ = ((sbus_buf[0] | (sbus_buf[1] << 8)) & 0x07ff) - RC_CH_VALUE_OFFSET; //!< Channel 0
    rc_ctrl[TEMP].rc.rocker_r1 = (((sbus_buf[1] >> 3) | (sbus_buf[2] << 5)) & 0x07ff) - RC_CH_VALUE_OFFSET;
    //!< Channel 1
    rc_ctrl[TEMP].rc.rocker_l_ = (((sbus_buf[2] >> 6) | (sbus_buf[3] << 2) | (sbus_buf[4] << 10)) & 0x07ff) -
                                 RC_CH_VALUE_OFFSET; //!< Channel 2
    rc_ctrl[TEMP].rc.rocker_l1 = (((sbus_buf[4] >> 1) | (sbus_buf[5] << 7)) & 0x07ff) - RC_CH_VALUE_OFFSET;
    //!< Channel 3
    rc_ctrl[TEMP].rc.dial = ((sbus_buf[16] | (sbus_buf[17] << 8)) & 0x07FF) - RC_CH_VALUE_OFFSET; // 左侧拨轮
    RectifyRCjoystick();
    // 开关,0左1右
    rc_ctrl[TEMP].rc.switch_right = ((sbus_buf[5] >> 4) & 0x0003); //!< Switch right
    rc_ctrl[TEMP].rc.switch_left = ((sbus_buf[5] >> 4) & 0x000C) >> 2; //!< Switch left

    // 鼠标解析
    rc_ctrl[TEMP].mouse.x = (sbus_buf[6] | (sbus_buf[7] << 8)); //!< Mouse X axis
    rc_ctrl[TEMP].mouse.y = (sbus_buf[8] | (sbus_buf[9] << 8)); //!< Mouse Y axis
    rc_ctrl[TEMP].mouse.z = (sbus_buf[10] | (sbus_buf[11] << 8)); //!< Mouse Z axis   // 滚轮
    rc_ctrl[TEMP].mouse.press_l = sbus_buf[12]; //!< Mouse Left Is Press ?
    rc_ctrl[TEMP].mouse.press_r = sbus_buf[13]; //!< Mouse Right Is Press ?

    //  位域的按键值解算,直接memcpy即可,注意小端低字节在前,即lsb在第一位,msb在最后
    *(uint16_t *) &rc_ctrl[TEMP].key[KEY_PRESS] = (uint16_t) (sbus_buf[14] | (sbus_buf[15] << 8));
    if (rc_ctrl[TEMP].key[KEY_PRESS].ctrl) // ctrl键按下
        rc_ctrl[TEMP].key[KEY_PRESS_WITH_CTRL] = rc_ctrl[TEMP].key[KEY_PRESS];
    else
        memset(&rc_ctrl[TEMP].key[KEY_PRESS_WITH_CTRL], 0, sizeof(Key_t));
    if (rc_ctrl[TEMP].key[KEY_PRESS].shift) // shift键按下
        rc_ctrl[TEMP].key[KEY_PRESS_WITH_SHIFT] = rc_ctrl[TEMP].key[KEY_PRESS];
    else
        memset(&rc_ctrl[TEMP].key[KEY_PRESS_WITH_SHIFT], 0, sizeof(Key_t));

    uint16_t key_now = rc_ctrl[TEMP].key[KEY_PRESS].keys, // 当前按键是否按下
            key_last = rc_ctrl[LAST].key[KEY_PRESS].keys, // 上一次按键是否按下
            key_with_ctrl = rc_ctrl[TEMP].key[KEY_PRESS_WITH_CTRL].keys, // 当前ctrl组合键是否按下
            key_with_shift = rc_ctrl[TEMP].key[KEY_PRESS_WITH_SHIFT].keys, //  当前shift组合键是否按下
            key_last_with_ctrl = rc_ctrl[LAST].key[KEY_PRESS_WITH_CTRL].keys, // 上一次ctrl组合键是否按下
            key_last_with_shift = rc_ctrl[LAST].key[KEY_PRESS_WITH_SHIFT].keys; // 上一次shift组合键是否按下

    for (uint16_t i = 0, j = 0x1; i < 16; j <<= 1, i++)
    {
        if (i == 4 || i == 5) // 4,5位为ctrl和shift,直接跳过
            continue;
        // 如果当前按键按下,上一次按键没有按下,且ctrl和shift组合键没有按下,则按键按下计数加1(检测到上升沿)
        if ((key_now & j) && !(key_last & j) && !(key_with_ctrl & j) && !(key_with_shift & j))
            rc_ctrl[TEMP].key_count[KEY_PRESS][i]++;
        // 当前ctrl组合键按下,上一次ctrl组合键没有按下,则ctrl组合键按下计数加1(检测到上升沿)
        if ((key_with_ctrl & j) && !(key_last_with_ctrl & j))
            rc_ctrl[TEMP].key_count[KEY_PRESS_WITH_CTRL][i]++;
        // 当前shift组合键按下,上一次shift组合键没有按下,则shift组合键按下计数加1(检测到上升沿)
        if ((key_with_shift & j) && !(key_last_with_shift & j))
            rc_ctrl[TEMP].key_count[KEY_PRESS_WITH_SHIFT][i]++;
    }

    memcpy(&rc_ctrl[LAST], &rc_ctrl[TEMP], sizeof(RC_ctrl_t)); // 保存上一次的数据,用于按键持续按下和切换的判断
}
#endif

#ifdef REMOTE_FS_I6X
/**
 * @brief 遥控器数据解析
 *
 * @param sbus_buf 接收buffer
 */
static void sbus_to_rc(const uint8_t *sbus_buf)
{
    // // 摇杆,直接解算时减去偏置
    // rc_ctrl[TEMP].rc.rocker_r_ = ((sbus_buf[0] | (sbus_buf[1] << 8)) & 0x07ff) - RC_CH_VALUE_OFFSET;                              //!< Channel 0
    // rc_ctrl[TEMP].rc.rocker_r1 = (((sbus_buf[1] >> 3) | (sbus_buf[2] << 5)) & 0x07ff) - RC_CH_VALUE_OFFSET;                       //!< Channel 1
    // rc_ctrl[TEMP].rc.rocker_l_ = (((sbus_buf[2] >> 6) | (sbus_buf[3] << 2) | (sbus_buf[4] << 10)) & 0x07ff) - RC_CH_VALUE_OFFSET; //!< Channel 2
    // rc_ctrl[TEMP].rc.rocker_l1 = (((sbus_buf[4] >> 1) | (sbus_buf[5] << 7)) & 0x07ff) - RC_CH_VALUE_OFFSET;                       //!< Channel 3
    // rc_ctrl[TEMP].rc.dial      = ((sbus_buf[16] | (sbus_buf[17] << 8)) & 0x07FF) - RC_CH_VALUE_OFFSET;                            // 左侧拨轮
    // RectifyRCjoystick();
    // // 开关,0左1右
    // rc_ctrl[TEMP].rc.switch_right = ((sbus_buf[5] >> 4) & 0x0003);      //!< Switch right
    // rc_ctrl[TEMP].rc.switch_left  = ((sbus_buf[5] >> 4) & 0x000C) >> 2; //!< Switch left

    // // 鼠标解析
    // rc_ctrl[TEMP].mouse.x       = (sbus_buf[6] | (sbus_buf[7] << 8)); //!< Mouse X axis
    // rc_ctrl[TEMP].mouse.y       = (sbus_buf[8] | (sbus_buf[9] << 8)); //!< Mouse Y axis
    // rc_ctrl[TEMP].mouse.z       = (sbus_buf[10] | (sbus_buf[11] << 8)); //!< Mouse Z axis   // 滚轮
    // rc_ctrl[TEMP].mouse.press_l = sbus_buf[12];                       //!< Mouse Left Is Press ?
    // rc_ctrl[TEMP].mouse.press_r = sbus_buf[13];                       //!< Mouse Right Is Press ?

    // //  位域的按键值解算,直接memcpy即可,注意小端低字节在前,即lsb在第一位,msb在最后
    // *(uint16_t *)&rc_ctrl[TEMP].key[KEY_PRESS] = (uint16_t)(sbus_buf[14] | (sbus_buf[15] << 8));
    // if (rc_ctrl[TEMP].key[KEY_PRESS].ctrl) // ctrl键按下
    //     rc_ctrl[TEMP].key[KEY_PRESS_WITH_CTRL] = rc_ctrl[TEMP].key[KEY_PRESS];
    // else
    //     memset(&rc_ctrl[TEMP].key[KEY_PRESS_WITH_CTRL], 0, sizeof(Key_t));
    // if (rc_ctrl[TEMP].key[KEY_PRESS].shift) // shift键按下
    //     rc_ctrl[TEMP].key[KEY_PRESS_WITH_SHIFT] = rc_ctrl[TEMP].key[KEY_PRESS];
    // else
    //     memset(&rc_ctrl[TEMP].key[KEY_PRESS_WITH_SHIFT], 0, sizeof(Key_t));

    // uint16_t key_now        = rc_ctrl[TEMP].key[KEY_PRESS].keys,            // 当前按键是否按下
    //     key_last            = rc_ctrl[LAST].key[KEY_PRESS].keys,            // 上一次按键是否按下
    //     key_with_ctrl       = rc_ctrl[TEMP].key[KEY_PRESS_WITH_CTRL].keys,  // 当前ctrl组合键是否按下
    //     key_with_shift      = rc_ctrl[TEMP].key[KEY_PRESS_WITH_SHIFT].keys, //  当前shift组合键是否按下
    //     key_last_with_ctrl  = rc_ctrl[LAST].key[KEY_PRESS_WITH_CTRL].keys,  // 上一次ctrl组合键是否按下
    //     key_last_with_shift = rc_ctrl[LAST].key[KEY_PRESS_WITH_SHIFT].keys; // 上一次shift组合键是否按下

    // for (uint16_t i = 0, j = 0x1; i < 16; j <<= 1, i++) {
    //     if (i == 4 || i == 5) // 4,5位为ctrl和shift,直接跳过
    //         continue;
    //     // 如果当前按键按下,上一次按键没有按下,且ctrl和shift组合键没有按下,则按键按下计数加1(检测到上升沿)
    //     if ((key_now & j) && !(key_last & j) && !(key_with_ctrl & j) && !(key_with_shift & j))
    //         rc_ctrl[TEMP].key_count[KEY_PRESS][i]++;
    //     // 当前ctrl组合键按下,上一次ctrl组合键没有按下,则ctrl组合键按下计数加1(检测到上升沿)
    //     if ((key_with_ctrl & j) && !(key_last_with_ctrl & j))
    //         rc_ctrl[TEMP].key_count[KEY_PRESS_WITH_CTRL][i]++;
    //     // 当前shift组合键按下,上一次shift组合键没有按下,则shift组合键按下计数加1(检测到上升沿)
    //     if ((key_with_shift & j) && !(key_last_with_shift & j))
    //         rc_ctrl[TEMP].key_count[KEY_PRESS_WITH_SHIFT][i]++;
    // }

    if ((sbus_buf[0] != SBUS_HEAD) || (sbus_buf[24] != SBUS_END))
        return;

    // if (sbus_buf[23] == 0x0C)
    // 	rc_ctrl->online = 0;
    // else
    // 	rc_ctrl->online = 1;

    rc_ctrl[TEMP].rc.ch[0] = ((sbus_buf[1] | sbus_buf[2] << 8) & 0x07FF); // 通道1 (云台yaw)
    rc_ctrl[TEMP].rc.ch[1] = ((sbus_buf[2] >> 3 | sbus_buf[3] << 5) & 0x07FF); // 通道2 (云台pitch)
    rc_ctrl[TEMP].rc.ch[2] = ((sbus_buf[3] >> 6 | sbus_buf[4] << 2 | sbus_buf[5] << 10) & 0x07FF); // 通道3 (前后)
    rc_ctrl[TEMP].rc.ch[3] = ((sbus_buf[5] >> 1 | sbus_buf[6] << 7) & 0x07FF); // 通道4 (左右)
    rc_ctrl[TEMP].rc.ch[4] = ((sbus_buf[6] >> 4 | sbus_buf[7] << 4) & 0x07FF); // 通道5 (SWA)
    rc_ctrl[TEMP].rc.ch[5] = ((sbus_buf[7] >> 7 | sbus_buf[8] << 1 | sbus_buf[9] << 9) & 0x07FF); // 通道6 (SWB)
    rc_ctrl[TEMP].rc.ch[6] = ((sbus_buf[9] >> 2 | sbus_buf[10] << 6) & 0x07FF); // 通道7 (SWC)
    rc_ctrl[TEMP].rc.ch[7] = ((sbus_buf[10] >> 5 | sbus_buf[11] << 3) & 0x07FF); // 通道8 (SWD)
    rc_ctrl[TEMP].rc.ch[8] = ((sbus_buf[12] | sbus_buf[13] << 8) & 0x07FF); // 通道9 (VrA旋钮)
    rc_ctrl[TEMP].rc.ch[9] = ((sbus_buf[13] >> 3 | sbus_buf[14] << 5) & 0x07FF); // 通道10 (VrB旋钮)

    // rc_ctrl[TEMP].rc.ch[0] = ((sbus_buf[0] | sbus_buf[1] << 8) & 0x07FF);					   // 通道1 (云台yaw)
    // rc_ctrl[TEMP].rc.ch[1] = ((sbus_buf[1] >> 3 | sbus_buf[2] << 5) & 0x07FF);				   // 通道2 (云台pitch)
    // rc_ctrl[TEMP].rc.ch[2] = ((sbus_buf[2] >> 6 | sbus_buf[3] << 2 | sbus_buf[4] << 10) & 0x07FF); // 通道3 (前后)
    // rc_ctrl[TEMP].rc.ch[3] = ((sbus_buf[4] >> 1 | sbus_buf[5] << 7) & 0x07FF);				   // 通道4 (左右)
    // rc_ctrl[TEMP].rc.ch[4] = ((sbus_buf[5] >> 4 | sbus_buf[6] << 4) & 0x07FF);				   // 通道5 (SWA)
    // rc_ctrl[TEMP].rc.ch[5] = ((sbus_buf[6] >> 7 | sbus_buf[7] << 1 | sbus_buf[8] << 9) & 0x07FF);  // 通道6 (SWB)
    // rc_ctrl[TEMP].rc.ch[6] = ((sbus_buf[8] >> 2 | sbus_buf[9] << 6) & 0x07FF);			   // 通道7 (SWC)
    // rc_ctrl[TEMP].rc.ch[7] = ((sbus_buf[9] >> 5 | sbus_buf[10] << 3) & 0x07FF);			   // 通道8 (SWD)
    // rc_ctrl[TEMP].rc.ch[8] = ((sbus_buf[11] | sbus_buf[12] << 8) & 0x07FF);				   // 通道9 (VrA旋钮)
    // rc_ctrl[TEMP].rc.ch[9] = ((sbus_buf[12] >> 3 | sbus_buf[13] << 5) & 0x07FF);			   // 通道10 (VrB旋钮)

    rc_ctrl[TEMP].sw_a = (rc_ctrl[TEMP].rc.ch[4] == 0x00F0) ? RC_SW_UP : RC_SW_DOWN;
    rc_ctrl[TEMP].sw_b = rc_ctrl[TEMP].rc.ch[5]; //3档
    rc_ctrl[TEMP].sw_c = rc_ctrl[TEMP].rc.ch[6]; //3档
    rc_ctrl[TEMP].sw_d = (rc_ctrl[TEMP].rc.ch[7] == 0x00F0) ? RC_SW_UP : RC_SW_DOWN;

    // 解析SWB状态
    if (rc_ctrl[TEMP].rc.ch[6] == 0x00F0)
    {
        rc_ctrl[TEMP].sw_b = RC_SW_UP;
    }
    else if (rc_ctrl[TEMP].rc.ch[6] == 0x0400)
    {
        rc_ctrl[TEMP].sw_b = RC_SW_MID;
    }
    else if (rc_ctrl[TEMP].rc.ch[6] == 0x070F)
    {
        rc_ctrl[TEMP].sw_b = RC_SW_DOWN;
    }
    else
    {
        rc_ctrl[TEMP].sw_b = 0;
    }

    if (rc_ctrl[TEMP].sw_b != rc_ctrl[TEMP].sw_b_last)
    {
        if (rc_ctrl[TEMP].sw_b == RC_SW_UP)
        {
            rc_ctrl[TEMP].sw_b_midtoup_flag = 1;
            rc_ctrl[TEMP].sw_b_uptomid_flag = 0;
            rc_ctrl[TEMP].sw_b_midtodown_flag = 0;
        }
        else if (rc_ctrl[TEMP].sw_b == RC_SW_MID)
        {
            rc_ctrl[TEMP].sw_b_midtoup_flag = 0;
            rc_ctrl[TEMP].sw_b_uptomid_flag = 1;
            rc_ctrl[TEMP].sw_b_midtodown_flag = 0;
        }
        else if (rc_ctrl[TEMP].sw_b == RC_SW_DOWN)
        {
            rc_ctrl[TEMP].sw_b_midtoup_flag = 0;
            rc_ctrl[TEMP].sw_b_uptomid_flag = 0;
            rc_ctrl[TEMP].sw_b_midtodown_flag = 1;
        }
    }
    rc_ctrl[TEMP].sw_b_last = rc_ctrl[TEMP].sw_b;

    // 解析SWC状态
    if (rc_ctrl[TEMP].rc.ch[6] == 0x00F0)
    {
        rc_ctrl[TEMP].sw_c = RC_SW_UP;
    }
    else if (rc_ctrl[TEMP].rc.ch[6] == 0x0400)
    {
        rc_ctrl[TEMP].sw_c = RC_SW_MID;
    }
    else if (rc_ctrl[TEMP].rc.ch[6] == 0x070F)
    {
        rc_ctrl[TEMP].sw_c = RC_SW_DOWN;
    }
    else
    {
        rc_ctrl[TEMP].sw_c = 0;
    }

    if (rc_ctrl[TEMP].sw_c != rc_ctrl[TEMP].sw_c_last)
    {
        if (rc_ctrl[TEMP].sw_c == RC_SW_UP)
        {
            rc_ctrl[TEMP].sw_c_midtoup_flag = 1;
            rc_ctrl[TEMP].sw_c_uptomid_flag = 0;
            rc_ctrl[TEMP].sw_c_midtodown_flag = 0;
        }
        else if (rc_ctrl[TEMP].sw_c == RC_SW_MID)
        {
            rc_ctrl[TEMP].sw_c_midtoup_flag = 0;
            rc_ctrl[TEMP].sw_c_uptomid_flag = 1;
            rc_ctrl[TEMP].sw_c_midtodown_flag = 0;
        }
        else if (rc_ctrl[TEMP].sw_c == RC_SW_DOWN)
        {
            rc_ctrl[TEMP].sw_c_midtoup_flag = 0;
            rc_ctrl[TEMP].sw_c_uptomid_flag = 0;
            rc_ctrl[TEMP].sw_c_midtodown_flag = 1;
        }
    }
    rc_ctrl[TEMP].sw_c_last = rc_ctrl[TEMP].sw_c;

    // SWA和SWB的状态变化
    if (rc_ctrl[TEMP].sw_a != rc_ctrl[TEMP].sw_a_last)
    {
        rc_ctrl[TEMP].sw_a_up_to_down_flag = (rc_ctrl[TEMP].sw_a == RC_SW_UP) ? 1 : 0;
    }
    rc_ctrl[TEMP].sw_a_last = rc_ctrl[TEMP].sw_a;

    // if (rc_ctrl[TEMP].sw_b != rc_ctrl[TEMP].sw_b_last) {
    //     rc_ctrl[TEMP].sw_b_up_to_down_flag = (rc_ctrl[TEMP].sw_b == RC_SW_UP) ? 1 : 0;
    // }
    // rc_ctrl[TEMP].sw_b_last = rc_ctrl[TEMP].sw_b;

    // SWD的状态变化
    if (rc_ctrl[TEMP].sw_d != rc_ctrl[TEMP].sw_d_last)
    {
        rc_ctrl[TEMP].sw_d_up_to_down_flag = (rc_ctrl[TEMP].sw_d == RC_SW_UP) ? 1 : 0;
    }
    rc_ctrl[TEMP].sw_d_last = rc_ctrl[TEMP].sw_d;

    // 修正 ch1~ch4
    rc_ctrl[TEMP].ch1 = rc_ctrl[TEMP].rc.ch[0] - 1024;
    rc_ctrl[TEMP].ch2 = rc_ctrl[TEMP].rc.ch[1] - 1024;
    rc_ctrl[TEMP].ch3 = rc_ctrl[TEMP].rc.ch[2] - 1024;
    rc_ctrl[TEMP].ch4 = rc_ctrl[TEMP].rc.ch[3] - 1024;
    // ...existing code...

    memcpy(&rc_ctrl[LAST], &rc_ctrl[TEMP], sizeof(RC_ctrl_t)); // 保存上一次的数据,用于按键持续按下和切换的判断
}
#endif

/**
 * @brief 对sbus_to_rc的简单封装,用于注册到bsp_usart的回调函数中
 *
 */
static void RemoteControlRxCallback()
{
    sbus_to_rc(rc_usart_instance->recv_buff); // 进行协议解析
    DaemonReload(rc_daemon_instance); // 先喂狗
}

/**
 * @brief 遥控器离线的回调函数,注册到守护进程中,串口掉线时调用
 *
 */
static void RCLostCallback(void *id)
{
    memset(rc_ctrl, 0, sizeof(rc_ctrl)); // 清空遥控器数据
    USARTServiceInit(rc_usart_instance); // 尝试重新启动接收

    RobotMode = REMOTE_NOT_CONNECTED;

    // LEDErrLog(0, LED_COLOR_R); // 红灯常亮 表示遥控器离线
}

RC_ctrl_t *RemoteControlInit(UART_HandleTypeDef *rc_usart_handle)
{
    USART_Init_Config_s conf;
    conf.module_callback = RemoteControlRxCallback;
    conf.usart_handle = rc_usart_handle;
    conf.recv_buff_size = REMOTE_CONTROL_FRAME_SIZE;

    rc_usart_instance = USARTRegister(&conf);

    // 进行守护进程的注册,用于定时检查遥控器是否正常工作
    Daemon_Init_Config_s daemon_conf = {
        .reload_count = 10, // 100ms未收到数据视为离线,遥控器的接收频率实际上是1000/14Hz(大约70Hz)
        .callback = RCLostCallback,
        .owner_id = NULL, // 只有1个遥控器,不需要owner_id
    };
    rc_daemon_instance = DaemonRegister(&daemon_conf);

    rc_init_flag = 1;
    return rc_ctrl;
}

uint8_t RemoteControlIsOnline()
{
    if (rc_init_flag)
        return DaemonIsOnline(rc_daemon_instance);
    return 0;
}
