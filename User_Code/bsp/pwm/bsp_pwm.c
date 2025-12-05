#include "bsp_pwm.h"
#include "stdlib.h"
#include "memory.h"

// 配合中断以及初始化
static uint8_t idx;
static PWMInstance *pwm_instance[PWM_DEVICE_CNT] = {NULL}; // 所有的pwm instance保存于此,用于callback时判断中断来源
static uint32_t PWMSelectTclk(TIM_HandleTypeDef *htim);

/**
 * @brief pwm dma传输完成回调函数
 *
 * @param htim 发生中断的定时器句柄
 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    for (uint8_t i = 0; i < idx; i++)
    {
        // 来自同一个定时器的中断且通道相同
        if (pwm_instance[i]->htim == htim && htim->Channel == (1 << (pwm_instance[i]->channel / 4)))
        {
            if (pwm_instance[i]->callback) // 如果有回调函数
                pwm_instance[i]->callback(pwm_instance[i]);
            return; // 一次只能有一个通道的中断,所以直接返回
        }
    }
}

PWMInstance *PWMRegister(PWM_Init_Config_s *config)
{
    if (idx >= PWM_DEVICE_CNT) // 超过最大实例数,考虑增加或查看是否有内存泄漏
        while (1);
    PWMInstance *pwm = (PWMInstance *) malloc(sizeof(PWMInstance));
    memset(pwm, 0, sizeof(PWMInstance));

    pwm->htim = config->htim;
    pwm->channel = config->channel;
    pwm->period = config->period;
    pwm->dutyratio = config->dutyratio;
    pwm->callback = config->callback;
    pwm->id = config->id;
    pwm->tclk = PWMSelectTclk(pwm->htim);
    // 启动PWM
    HAL_TIM_PWM_Start(pwm->htim, pwm->channel);
    PWMSetPeriod(pwm, pwm->period);
    PWMSetDutyRatio(pwm, pwm->dutyratio);
    pwm_instance[idx++] = pwm;
    return pwm;
}

/* 只是对HAL的函数进行了形式上的封装 */
void PWMStart(PWMInstance *pwm)
{
    HAL_TIM_PWM_Start(pwm->htim, pwm->channel);
}

/* 只是对HAL的函数进行了形式上的封装 */
void PWMStop(PWMInstance *pwm)
{
    HAL_TIM_PWM_Stop(pwm->htim, pwm->channel);
}

/*
 * @brief 设置pwm周期
 *
 * @param pwm pwm实例
 * @param period 周期 单位 s
 */
void PWMSetPeriod(PWMInstance *pwm, float period)
{
    __HAL_TIM_SetAutoreload(pwm->htim, period * ((pwm->tclk) / (pwm->htim->Init.Prescaler + 1)));
}

/*
    * @brief 设置pwm占空比
    *
    * @param pwm pwm实例
    * @param dutyratio 占空比 0~1
*/
void PWMSetDutyRatio(PWMInstance *pwm, float dutyratio)
{
    __HAL_TIM_SetCompare(pwm->htim, pwm->channel, dutyratio * (pwm->htim->Instance->ARR));
}

/* 只是对HAL的函数进行了形式上的封装 */
void PWMStartDMA(PWMInstance *pwm, uint32_t *pData, uint32_t Size)
{
    HAL_TIM_PWM_Start_DMA(pwm->htim, pwm->channel, pData, Size);
}

// STM32H7 PWM时钟源选择函数（简洁版）
static uint32_t PWMSelectTclk(TIM_HandleTypeDef *htim)
{
    // 获取APB预分频器值
    uint32_t ppre1 = ((RCC->D2CFGR & RCC_D2CFGR_D2PPRE1) >> RCC_D2CFGR_D2PPRE1_Pos);
    uint32_t ppre2 = ((RCC->D2CFGR & RCC_D2CFGR_D2PPRE2) >> RCC_D2CFGR_D2PPRE2_Pos);

    // 计算倍频因子
    uint32_t tim_mult1 = (ppre1 == 0b000 || ppre1 == 0b100) ? 1 : 2;
    uint32_t tim_mult2 = (ppre2 == 0b000 || ppre2 == 0b100) ? 1 : 2;

    // 根据定时器实例判断
    TIM_TypeDef *tim = htim->Instance;

    if (tim == TIM1 || tim == TIM8 || tim == TIM15 ||
        tim == TIM16 || tim == TIM17) // 某些H7型号有TIM20
    {
        // APB2上的定时器
        return HAL_RCC_GetPCLK2Freq() * tim_mult2;
    }
    else if (tim == TIM2 || tim == TIM3 || tim == TIM4 ||
             tim == TIM5 || tim == TIM12 || tim == TIM13 || tim == TIM14 || tim == TIM6 || tim == TIM7)
    {
        // APB1上的定时器
        return HAL_RCC_GetPCLK1Freq() * tim_mult1;
    }
    else if (tim == TIM23 || tim == TIM24) // 某些H7型号有这些定时器
    {
    }

    return 0; // 未识别的定时器
}
