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

#ifdef __cplusplus
extern "C"
{
#endif
#include "buzzer.h"
#include "cmsis_os.h"
#include "tim.h"
#include "delayticks.h"
#include "rc.h"

#ifdef __cplusplus
}
#endif

#include <ems_parser.hpp>


using namespace ems::literals;
constexpr auto spring =
		R"((400)
        2s`,,1s`,7,,1s`,2s`-,3`-2s`,1s`,,,
        2s`,,1s`,7,,1s`,2s`-,3`-2s`,1s`,,,
        2s`,,1s`,7,,1s`,2s`-,3`-2s`,1s`,,,
        2s`,,1s`,7,,1s`,2s`-,3`-2s`,1s`,,7-1s`-
        2s`,2s`,1s`,3`,2s`,1s`,1s`,1s`,7,3`,2s`,1s`,
        1s`,,7-1s`-2s`,,,,,,7,4s`,7`,
        6s`,,7`,6s`,,7`,6s`-5s`-4s`,,4s`,1s`,3`,
        3`,2s`,2s`,2s`,,,3`,2s`,1s`,2s`,,4s`,
        7,,,
)"_ems;

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
const uint16_t music_steps[3][7] = {
	{262, 294, 330, 349, 392, 440, 494},
	{523, 587, 659, 698, 784, 880, 988},
	{1046, 1175, 1318, 1397, 1568, 1760, 1976}
};

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


/*---------------------FUNCTIONS---------------------*/

#define BUZZER_TIM_CLK 5000000U

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
	uint16_t period = BUZZER_TIM_CLK / freq - 1;
	__HAL_TIM_SET_AUTORELOAD(&htim12, period);
	__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, period / 2);
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
	__HAL_TIM_DISABLE(&htim12);
	htim12.Instance->CNT = 0;
	uint32_t arr = BUZZER_TIM_CLK / note - 1;
	htim12.Instance->ARR = arr;
	htim12.Instance->CCR2 = (uint32_t) ((float) arr * volume * 0.5f);
	__HAL_TIM_ENABLE(&htim12);
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
	HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2);

	// 低音域 C4~B4
	buzzer_note(262, 0.5);
	HAL_Delay(150);
	buzzer_note(294, 0.5);
	HAL_Delay(150);
	buzzer_note(330, 0.5);
	HAL_Delay(150);
	buzzer_note(349, 0.5);
	HAL_Delay(150);
	buzzer_note(392, 0.5);
	HAL_Delay(150);
	buzzer_note(440, 0.5);
	HAL_Delay(150);
	buzzer_note(494, 0.5);
	HAL_Delay(150);
	// 中音域 C5~B5
	buzzer_note(523, 0.5);
	HAL_Delay(150);
	buzzer_note(587, 0.5);
	HAL_Delay(150);
	buzzer_note(659, 0.5);
	HAL_Delay(150);
	buzzer_note(698, 0.5);
	HAL_Delay(150);
	buzzer_note(784, 0.5);
	HAL_Delay(150);
	buzzer_note(880, 0.5);
	HAL_Delay(150);
	buzzer_note(988, 0.5);
	HAL_Delay(150);
	// 高音域 C6~B6
	buzzer_note(1046, 0.5);
	HAL_Delay(150);
	buzzer_note(1175, 0.5);
	HAL_Delay(150);
	buzzer_note(1318, 0.5);
	HAL_Delay(150);
	buzzer_note(1397, 0.5);
	HAL_Delay(150);
	buzzer_note(1568, 0.5);
	HAL_Delay(150);
	buzzer_note(1760, 0.5);
	HAL_Delay(150);
	buzzer_note(1976, 0.5);
	HAL_Delay(400);

	buzzer_off();
}

void SongSpring(void) //春日影！！！
{
	for (auto [ratio, duration_ms]: spring)
	{
		const auto note = static_cast<uint16_t>(ratio * 50); // 523.25Hz是中音do的频率
		buzzer_note(note, 0.5f);
		HAL_Delay(duration_ms);
		// buzzer_off();
		// HAL_Delay(1);
	}
}

void buzzerTask(void const *argument)
{
	(void) argument;
	HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2);

	for (;;)
	{
		if (!RemoteControlIsOnline())
		{
			buzzer_note(659, 0.5); // E5
			osDelay(250);
			buzzer_note(494, 0.5); // B4
			osDelay(250);
			buzzer_off();
			osDelay(500);
		}
		else
		{
			osDelay(200);
		}
	}
}

void SongLaoda(void)
{
	buzzer_note(494, 0.5); // B4
	HAL_Delay(200);
	buzzer_note(740, 0.5); // F#5
	HAL_Delay(200);
	buzzer_note(659, 0.5); // E5
	HAL_Delay(200);
	buzzer_note(494, 0.5); // B4
	HAL_Delay(500);
	buzzer_note(659, 0.5); // E5
	HAL_Delay(170);
	buzzer_note(740, 0.5); // F#5
	HAL_Delay(170);
	buzzer_note(831, 0.5); // G#5
	HAL_Delay(170);
	buzzer_note(740, 0.5); // F#5
	HAL_Delay(170);
	buzzer_note(659, 0.5); // E5
	HAL_Delay(170);
	buzzer_note(740, 0.5); // F#5
	HAL_Delay(170);
	buzzer_note(494, 0.5); // B4
	HAL_Delay(215);
	buzzer_note(740, 0.5); // F#5
	HAL_Delay(215);
	buzzer_note(659, 0.5); // E5
	HAL_Delay(215);
	buzzer_note(494, 0.5); // B4
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
