/**
 * @file buzzer.c
 * @brief 蜂鸣器控制
 * @author TuxMonkey
 * @version v1.0-bsp
 * @date 2025-11-15
 *
 * @copyright Copyright (c) 2025 DLMU-C.ONE
 *
 * @par 修改日志:
 * <table>
 * <tr><th>Date       <th>Version      <th>Author        <th>Description
 * <tr><td>25-11.15   <td>v1.0-bsp     <td>TuxMonkey     <td>蜂鸣器部分
 * </table>
 */

/*---------------------INCLUDES---------------------*/
#include "buzzer.h"
#include "cmsis_os.h"
#include "tim.h"
#include "delayticks.h"

/*---------------------VARIABLES---------------------*/
//
// uint8_t buzzer_state = PLAYING_INIT_MUSIC; //当前蜂鸣器播放状态

//低音
//1 - do - 262Hz
//2 - re - 294Hz
//3 - mi - 330Hz
//4 - fa - 349Hz
//5 - so - 392Hz
//6 - la - 440Hz
//7 - si - 494Hz
//中音
//1 - do - 523Hz
//2 - re - 587Hz
//3 - mi - 659Hz - 50
//4 - fa - 698Hz - 53
//5 - so - 784Hz - 60
//6 - la - 880Hz - 63
//7 - si - 988Hz - 66
//高音
//1 - do - 1046Hz - 70
//2 - re - 1175Hz
//3 - mi - 1318Hz
//4 - fa - 1397Hz
//5 - so - 1568Hz
//6 - la - 1760Hz
//7 - si - 1976Hz

//高中低音的实际频率 Hz
const uint16_t music_steps[3][7] = {{262, 294, 330, 349, 392, 440, 494},
                                    {523, 587, 659, 698, 784, 880, 988},
                                    {1046, 1175, 1318, 1397, 1568, 1760, 1976}};

//乐谱1:小星星
/*const uint16_t song_littlestar[42] = {0x1411,0x1411,0x1415,0x1415,0x1416,0x1416,0x1815,
																			0x1414,0x1414,0x1413,0x1413,0x1412,0x1412,0x1811,
																			0x1415,0x1415,0x1414,0x1414,0x1413,0x1413,0x1812,
																			0x1415,0x1415,0x1414,0x1414,0x1413,0x1413,0x1812,
																			0x1411,0x1411,0x1415,0x1415,0x1416,0x1416,0x1815,
																			0x1414,0x1414,0x1413,0x1413,0x1412,0x1412,0x1811};*/

//乐谱2:生日快乐
/*const uint16_t song_happybirthday[25] = {0x1205,0x1205,0x1406,0x1405,0x1411,0x1807,
																				 0x1205,0x1205,0x1406,0x1405,0x1412,0x1811,
																				 0x1205,0x1205,0x1415,0x1413,0x1411,0x0407,0x1406,
																				 0x1614,0x1214,0x1413,0x1411,0x1412,0x1811};*/

//乐谱3:东方红
/*const uint16_t song_eastred[41] = {0x1415,0x0215,0x1216,0x1812,0x1411,0x0211,0x1206,0x1812,
																	 0x1415,0x1415,0x0216,0x1221,0x1216,0x1215,0x1411,0x0211,0x1206,0x1812,
																	 0x1415,0x1412,0x1411,0x0207,0x1206,0x1405,0x1415,0x1412,0x1213,0x1212,0x1411,0x0211,0x1206,
																	 0x1212,0x1213,0x1212,0x1211,0x0212,0x1211,0x0207,0x1206,0x0405,0x1805};*/

//乐谱4:机甲大师主题曲—你
/*const uint16_t song_robomasteryou[148] = {0x1213,0x1213,0x1112,0x0113,0x1213,0x1212,0x1213,0x0215,
																					0x1215,0x1213,0x1213,0x1112,0x0113,0x1213,0x1212,0x1213,0x0215,
																					0x1215,0x0211,0x1411,0x8001,
																					0x1213,0x1213,0x1112,0x0113,0x1213,0x1212,0x1211,0x0212,
																					0x1212,0x1213,0x1213,0x1112,0x0113,0x1213,0x1212,0x1213,0x0215,
																					0x1215,0x1213,0x1213,0x1112,0x0113,0x1213,0x1212,0x1213,0x0221,
																					0x1221,0x0211,0x1411,0x8001,
																					0x1213,0x1213,0x1112,0x0113,0x1213,0x1212,0x1207,0x0106,0x1105,
																					0x0405,0x1105,0x1105,0x1105,0x1105,0x1415,0x1313,0x1112,
																					0x1212,0x0211,0x1211,0x1211,0x1211,0x1212,0x1213,0x0206,
																					0x1406,0x8001,0x1206,0x1213,0x1212,0x1211,0x0212,
																					0x1812,0x1413,0x1414,
																					0x0415,0x1215,0x1213,0x1215,0x1113,0x0115,0x1215,0x0217,
																					0x1217,0x0221,0x1221,0x1211,0x1212,0x1213,0x0215,
																					0x1215,0x0216,0x1216,0x1115,0x0116,0x1216,0x1115,0x0115,0x1115,0x0215,
																					0x1212,0x0212,0x1412,0x1413,0x1414,
																					0x0415,0x1215,0x1213,0x1215,0x1113,0x0115,0x1115,0x0217,
																					0x1217,0x0221,0x1421,0x1211,0x1212,0x1213,0x0217,
																					0x1217,0x0216,0x1416,0x1216,0x1115,0x0116,0x1116,0x0221,
																					0x1221,0x0222,0x1422,0x8001,0x1215,0x1221,0x1117,0x0121,0x1821};*/

//乐谱4-1:机甲大师主题曲—你(部分)
/*cosnt uint16_t song_robomasteryou2[66] = {0x1413,0x1414,0x0415,0x1215,0x1213,0x1215,0x1113,0x0115,0x1215,0x0217,
																					0x1217,0x0221,0x1421,0x1211,0x1212,0x1213,0x0215,
																					0x1215,0x0216,0x1216,0x1115,0x0116,0x1216,0x1115,0x0115,0x1215,0x0215,
																					0x1212,0x0212,0x1412,0x4001,0x1413,0x1414,
																					0x0415,0x1215,0x1213,0x1215,0x1113,0x0115,0x1215,0x0217,
																					0x1217,0x0221,0x1421,0x4001,0x1211,0x1212,0x1213,0x0217,
																					0x1217,0x0216,0x1416,0x1216,0x1115,0x0116,0x1216,0x0221,
																					0x1221,0x0222,0x1422,0x8001,0x1215,0x1221,0x1117,0x0121,0x1821};*/

//乐谱4-2:机甲大师主题曲—你(舔狗之歌)
const uint16_t song_robomasterlickdog[3] = {0x1411, 0x1412, 0x0415};
//DJI
//const uint16_t song_starting[3] = {};const uint16_t song_robomasterlickdog[13] = {0x1411,0x1414,0x0415,0x1215,0x1213,0x1215,0x1113,0x0115,0x1215,0x0217,0x1217,0x0221,0x1421};//125

//提示音
const uint8_t sound_warning[6] = {0x4B, 0x40, 0x4B, 0x40, 0x4B, 0x40}; //B__B__B__
const uint8_t sound_error[6] = {0x2B, 0x10, 0x2B, 0x10, 0x2B, 0x10}; //B_B_B_


const uint8_t sound_gyrocalibrating[6] = {0x2D, 0x40, 0x2D, 0x40, 0x2D, 0x40}; //D__D__D__
const uint8_t sound_autoaiming[6] = {0x2D, 0x10, 0x2D, 0x10, 0x2D, 0x10}; //D_D_D_

//
static uint16_t bzply_n = 0; //乐谱播放位置计数
static uint8_t bzply_count = 1; //单个音的节拍延时计数

auto note = 80;
/*---------------------FUNCTIONS---------------------*/

/***********************************************************************
** 函 数 名： SetBuzzerOff()
** 函数说明： 关闭蜂鸣器
**---------------------------------------------------------------------
** 输入参数： 无
** 返回参数： 无
***********************************************************************/
void SetBuzzerOff(void)
{
    __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 0);
}

/***********************************************************************
** 函 数 名： SetBuzzerFrequence()
** 函数说明： 设置蜂鸣器频率
**---------------------------------------------------------------------
** 输入参数： 频率 Hz
** 返回参数： 无
***********************************************************************/
void SetBuzzerFrequence(uint16_t freq)
{
    //buzzer --> tim12.channel2
    //分频后为1000000Hz
    uint16_t period = 1000000 / freq - 1;

    __HAL_TIM_SET_AUTORELOAD(&htim12, period);
    __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, period/2);

}

/***********************************************************************
** 函 数 名： buzzer_on()
** 函数说明： 播放歌曲(播放完自动停止,需要放在50Hz的定时器中断执行)
**---------------------------------------------------------------------
** 输入参数： psc,pwm
** 返回参数： 无
***********************************************************************/
void buzzer_on(uint16_t psc, uint16_t pwm)
{
    __HAL_TIM_PRESCALER(&htim12, psc);
    __HAL_TIM_SetCompare(&htim12, TIM_CHANNEL_2, pwm);
}

/***********************************************************************
** 函 数 名： buzzer_off()
** 函数说明： 设置蜂鸣器为关闭状态
**---------------------------------------------------------------------
** 输入参数： 无
** 返回参数： 无
***********************************************************************/
void buzzer_off(void)
{
    __HAL_TIM_SetCompare(&htim12, TIM_CHANNEL_2, 0);
}

/**
 * @brief 用蜂鸣器发出音符
 * @param note note
 * @param volume 音量，范围[0.0\\~1.0],用百分比表示
 */
void buzzer_note(uint16_t note, float volume)
{
    if (volume > 1.0f)
    {
        volume = 1.0f;
    }
    else if (volume < 0.0f)
    {
        volume = 0.0f;
    }
    // 禁用定时器
    __HAL_TIM_DISABLE(&htim12);
    // 重置定时器计数器
    htim12.Instance->CNT = 0;
    // 设置自动重装载寄存器（ARR），以控制PWM信号的频率
    htim12.Instance->ARR = (1000000 / note - 1) * 1u;
    // 设置比较寄存器（CCR3），以控制PWM信号的占空比
    htim12.Instance->CCR3 = (8 * 10500 / note - 1) * volume * 1u;
    // 重新启用定时器
    __HAL_TIM_ENABLE(&htim12);
    // 启动PWM信号
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2);
}

/**
 * @brief 启动音乐（简陋方式实现）
 *
 * 该函数通过一系列蜂鸣器发声和延时的组合来播放一首简单的歌曲。
 * 使用了固定的音高和延时时间，来模拟音乐的旋律。
 */
void systemstart_song(void)
{
    // 播放歌曲的旋律，每个音符后面都跟随一个延时
    // buzzer_note(50,0.5);
    // delay_ticks(450);
    // buzzer_note(53,0.5);
    // delay_ticks(450);
    // buzzer_note(60,0.5);
    // delay_ticks(750);
    // buzzer_note(50,0.5);
    // delay_ticks(250);
    // buzzer_note(60,0.5);
    // delay_ticks(250);
    // buzzer_note(50,0.5);
    // delay_ticks(290);
    // buzzer_note(60,0.5);
    // delay_ticks(300);
    // buzzer_note(75,0.5);
    // delay_ticks(550);
    // buzzer_note(80,0.5);
    // delay_ticks(1000);
    // 播放结束后关闭蜂鸣器
    // buzzer_off();

    // buzzer_note(85,0.5);//中音mi
    // delay_ticks(450);
    // buzzer_note(90,0.5);
    // delay_ticks(450);
    // buzzer_note(100,0.5);
    // delay_ticks(750);
    // buzzer_note(85,0.5);
    // delay_ticks(250);
    // buzzer_note(100,0.5);
    // delay_ticks(250);
    // buzzer_note(85,0.5);
    // delay_ticks(290);
    // buzzer_note(100,0.5);
    // delay_ticks(300);
    // buzzer_note(125,0.5);
    // delay_ticks(550);
    // buzzer_note(132,0.5);
    // delay_ticks(1000);

    // buzzer_note(135,0.5);//高音mi
    // delay_ticks(450);
    // buzzer_note(140,0.5);
    // delay_ticks(450);
    // buzzer_note(160,0.5);
    // delay_ticks(750);
    // buzzer_note(135,0.5);
    // delay_ticks(250);
    // buzzer_note(160,0.5);
    // delay_ticks(250);
    // buzzer_note(135,0.5);
    // delay_ticks(290);
    // buzzer_note(160,0.5);
    // delay_ticks(300);
    // buzzer_note(200,0.5);
    // delay_ticks(550);
    // buzzer_note(210,0.5);
    // delay_ticks(1000);

    //DJI
    // buzzer_note(80, 0.5); //高音do
    // HAL_Delay(450);
    // buzzer_note(90, 0.5); //高音re
    // HAL_Delay(450);
    // buzzer_note(120, 0.5); //高音sol
    // HAL_Delay(550);

    SongSpring(); //为什么要演奏春日影！！！
    // SongNortheastern();

    // SongLaoda();

    buzzer_off();
}

void SongSpring(void) //春日影！！！
{
    buzzer_note(165, 0.5); //mi
    HAL_Delay(280);
    buzzer_note(150, 0.5); //re
    HAL_Delay(280);
    buzzer_note(135, 0.5); //do
    HAL_Delay(280);
    buzzer_note(150, 0.5); //re
    HAL_Delay(280);
    buzzer_note(165, 0.5); //mi
    HAL_Delay(280);
    buzzer_note(170, 0.5); //fa
    HAL_Delay(200);
    buzzer_note(165, 0.5); //mi
    HAL_Delay(200);
    buzzer_note(150, 0.5); //re
    HAL_Delay(300);
}

void SongNortheastern(void)
{
    // buzzer_note(110, 0.5); //la
    // HAL_Delay(290);
    // buzzer_note(110, 0.5); //la
    // HAL_Delay(290);
    // buzzer_note(135, 0.5); //mi
    // HAL_Delay(290);
    // buzzer_note(160, 0.5); //so
    // HAL_Delay(290);
    // buzzer_note(135, 0.5); //mi
    // HAL_Delay(290);
    // buzzer_note(135, 0.5); //mi
    // HAL_Delay(290);
    // buzzer_note(138, 0.5); //so
    // HAL_Delay(290);
    // buzzer_note(135, 0.5); //mi
    // HAL_Delay(290);
    // buzzer_note(145, 0.5); //xi
    // HAL_Delay(290);
    // buzzer_note(110, 0.5); //la
    // HAL_Delay(290);

    // buzzer_note(85,0.5);//中音mi
    // delay_ticks(450);
    // buzzer_note(90,0.5);//fa
    // delay_ticks(450);
    // buzzer_note(100,0.5);//so
    // delay_ticks(750);
    // buzzer_note(85,0.5);//mi
    // delay_ticks(250);
    // buzzer_note(100,0.5);//so
    // delay_ticks(250);
    // buzzer_note(85,0.5);//mi
    // delay_ticks(290);
    // buzzer_note(100,0.5);//so
    // delay_ticks(300);
    // buzzer_note(125,0.5);//xi
    // delay_ticks(550);
    // buzzer_note(132,0.5);//~do
    // delay_ticks(1000);
}

void SongLaoda(void)
{
    // 播放歌曲牢大
    // buzzer_note(85,0.5);//中音mi
    // delay_ticks(450);
    // buzzer_note(90,0.5);
    // delay_ticks(450);
    buzzer_note(100, 0.5); //so
    HAL_Delay(200);
    buzzer_note(150, 0.5); //re
    HAL_Delay(200);
    buzzer_note(135, 0.5); //do
    HAL_Delay(200);
    buzzer_note(100, 0.5); //so
    HAL_Delay(500);
    buzzer_note(135, 0.5); //do
    HAL_Delay(170);
    buzzer_note(150, 0.5); //re
    HAL_Delay(170);
    buzzer_note(165, 0.5); //mi
    HAL_Delay(170);
    buzzer_note(150, 0.5); //re
    HAL_Delay(170);
    buzzer_note(135, 0.5); //do
    HAL_Delay(170);
    buzzer_note(150, 0.5); //re
    HAL_Delay(170);
    buzzer_note(100, 0.5); //so
    HAL_Delay(215);
    buzzer_note(150, 0.5); //re
    HAL_Delay(215);
    buzzer_note(135, 0.5); //do
    HAL_Delay(215);
    buzzer_note(100, 0.5); //so
    HAL_Delay(215);

    // buzzer_note(45,0.5);//do
    // HAL_Delay(150);
    // buzzer_note(50,0.5);//mi
    // HAL_Delay(250);
    // buzzer_note(60,0.5);//so
    // HAL_Delay(250);
    // buzzer_note(65,0.5);//la
    // HAL_Delay(250);
    // buzzer_note(60,0.5);//so
    // HAL_Delay(500);

    // buzzer_note(45,0.5);//do
    // HAL_Delay(250);
    // buzzer_note(46,0.5);//re
    // HAL_Delay(250);
    // buzzer_note(46,0.5);//re
    // HAL_Delay(250);
    // buzzer_note(45,0.5);//do
    // HAL_Delay(250);
    // buzzer_note(50,0.5);//mi
    // HAL_Delay(500);

    // buzzer_note(50,0.5);//mi
    // HAL_Delay(200);
    // buzzer_note(60,0.5);//so
    // HAL_Delay(250);
    // buzzer_note(65,0.5);//la
    // HAL_Delay(250);
    // buzzer_note(72,0.5);//si
    // HAL_Delay(250);
    // buzzer_note(65,0.5);//la
    // HAL_Delay(250);
    // buzzer_note(60,0.5);//so
    // HAL_Delay(250);
    // buzzer_note(50,0.5);//mi
    // HAL_Delay(200);
    // buzzer_note(46,0.5);//re
    // HAL_Delay(250);
    // buzzer_note(46,0.5);//re
    // HAL_Delay(250);
    // buzzer_note(45,0.5);//do
    // HAL_Delay(250);

    // buzzer_note(46,0.5);//re
    // HAL_Delay(250);
    // buzzer_note(46,0.5);//re
    // HAL_Delay(250);
    // buzzer_note(50,0.5);//mi
    // HAL_Delay(250);
    // buzzer_note(45,0.5);//do
    // HAL_Delay(250);
}
