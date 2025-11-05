/**
 * @file ws2812Task.c
 * @brief WS2812 LED control task
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

//真的有点抽象了不太清楚什么问题，24V的时候用16的分频器可以 5V时候就只能用32的分频器了 spi6的 这个之后注意一下吧 如果要开发还是用32的分频器。

/*---------------------INCLUDES----------------------*/
#include "ws2812status.h"
#include "ws2812.h"
#include "cmsis_os.h"
#include "tim.h"
#include "delayticks.h"

#include "robot_def.h"

/*---------------------VARIABLES---------------------*/
uint8_t r = 0;
uint8_t g = 0;
uint8_t b = 0;

/*---------------------FUNCTIONS---------------------*/
/**
 * @brief WS2812 LED control task
 * @param  argument
 */
void ws2812Task(void *argument) {
        while (1) {
                switch (RobotMode) {
                        case NORMAL_MODE:
                                BlinkGreen();
                                break;
                        case SYS_ERROR_OCCURRED:
                                BlinkRed();
                                break;
                        case REMOTE_NOT_CONNECTED:
                                BlinkYellow();
                                break;
                        case REMOTE_CONNECTED:
                                BlinkBlue();
                                break;
                        case AUTO_SHOOTING_MODE:
                                BlinkCyan();
                                break;
                        case IMU_CALIBERATION_MODE:
                                BlinkPurple();
                                break;
                        default:
                                BlinkYellow();
                }
                osDelay(10); // 每个任务最后都要写在while里，要不然会导致任务切换失败
        }
}

void BlinkYellow(void) {
        r = 255; // 红色分量
        g = 255; // 绿色分量
        b = 0; // 蓝色分量
        WS2812_Ctrl(r, g, b); // 设置为黄色
        delay_ticks(500); // 延时500毫秒
        WS2812_Ctrl(0, 0, 0); // 关闭LED灯
        delay_ticks(500); // 延时500毫秒
}

void BlinkRed(void) {
        r = 255; // 红色分量
        g = 0; // 绿色分量
        b = 0; // 蓝色分量
        WS2812_Ctrl(r, g, b); // 设置为红色
        delay_ticks(500); // 延时500毫秒
        WS2812_Ctrl(0, 0, 0); // 关闭LED灯
        delay_ticks(500); // 延时500毫秒
}

void BlinkGreen(void) {
        r = 0; // 红色分量
        g = 255; // 绿色分量
        b = 0; // 蓝色分量
        WS2812_Ctrl(r, g, b); // 设置为绿色
        delay_ticks(500); // 延时500毫秒
        WS2812_Ctrl(0, 0, 0); // 关闭LED灯
        delay_ticks(500); // 延时500毫秒
}

void BlinkBlue(void) {
        r = 0; // 红色分量
        g = 0; // 绿色分量
        b = 255; // 蓝色分量
        WS2812_Ctrl(r, g, b); // 设置为蓝色
        delay_ticks(500); // 延时500毫秒
        WS2812_Ctrl(0, 0, 0); // 关闭LED灯
        delay_ticks(500); // 延时500毫秒
}

void BlinkWhite(void) {
        r = 255; // 红色分量
        g = 255; // 绿色分量
        b = 255; // 蓝色分量
        WS2812_Ctrl(r, g, b); // 设置为白色
        delay_ticks(500); // 延时500毫秒
        WS2812_Ctrl(0, 0, 0); // 关闭LED灯
        delay_ticks(500); // 延时500毫秒
}

void BlinkCyan(void) {
        r = 0; // 红色分量
        g = 255; // 绿色分量
        b = 255; // 蓝色分量
        WS2812_Ctrl(r, g, b); // 设置为青色
        delay_ticks(500); // 延时500毫秒
        WS2812_Ctrl(0, 0, 0); // 关闭LED灯
        delay_ticks(500); // 延时500毫秒
}

void BlinkPurple(void) {
        r = 255; // 红色分量
        g = 0; // 绿色分量
        b = 255; // 蓝色分量
        WS2812_Ctrl(r, g, b); // 设置为紫色
        delay_ticks(500); // 延时500毫秒
        WS2812_Ctrl(0, 0, 0); // 关闭LED灯
        delay_ticks(500); // 延时500毫秒
}

void PWMControwLed(void) {
        //

        // WS2812_Ctrl(r, g, b);
        // r++;
        // g += 5;
        // b += 10;

        // r=0;g=255;b=0;//绿色
        // WS2812_Ctrl(r, g, b);
        // delay_ticks(500); // 使用延时函数代替osDelay，避免任务切换失败

        // r=0;g=0;b=255;//蓝色
        // WS2812_Ctrl(r, g, b);
        // delay_ticks(500);

        // r=0;g=255;b=255;//青色
        // WS2812_Ctrl(r, g, b);
        // delay_ticks(500);

        // r=255;g=0;b=255;//紫色
        // WS2812_Ctrl(r, g, b);
        // delay_ticks(500);

        // r=255;g=255;b=255;
        // WS2812_Ctrl(r, g, b);
        // delay_ticks(1000);
        // osDelay(1);
        // osDelay(100);
}
