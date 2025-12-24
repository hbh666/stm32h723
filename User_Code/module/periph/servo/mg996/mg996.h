/**
* @file mg996.h
    * @author TuxMonkey (https://github.com/TuxMonkey)
    * @brief MG996 servo motor driver header (MG996舵机驱动头文件)
    * @version 1.0
    * @date 2024-06-10
**/

#ifndef MG996_H
#define MG996_H

#ifdef __cplusplus
extern "C"
{



#endif

#include <stdint.h>

// 前向声明PWM实例结构体，与bsp_pwm.h中的类型保持一致
typedef struct PWMInstance PWMInstance;

/**
 * @brief MG996舵机初始化配置结构体
 */
typedef struct
{
    void *pwm_config; /**< PWM配置参数 */
    float min_dutyratio; /**< 最小占空比 (0.0 - 1.0) */
    float max_dutyratio; /**< 最大占空比 (0.0 - 1.0) */
    float min_angle; /**< 最小角度 (度) */
    float max_angle; /**< 最大角度 (度) */
} MG996_Init_Config_s;

/**
 * @brief MG996舵机实例结构体
 */
typedef struct
{
    PWMInstance *pwm; /**< PWM控制器实例指针，类型与bsp_pwm.h中的PWMInstance一致 */
    float min_dutyratio; /**< 最小占空比 */
    float max_dutyratio; /**< 最大占空比 */
    float min_angle; /**< 最小角度 (度) */
    float max_angle; /**< 最大角度 (度) */
} MG996Instance;

/**
 * @brief 注册并初始化MG996舵机
 *
 * @param config 初始化配置参数
 * @return MG996Instance* 舵机实例指针，失败返回NULL
 */
MG996Instance *MG996Register(MG996_Init_Config_s *config);

/**
 * @brief 设置舵机角度
 *
 * @param mg996 舵机实例指针
 * @param angle 目标角度 (度)
 */
void MG996SetAngle(MG996Instance *mg996, float angle);

#ifdef __cplusplus
}
#endif

#endif /* MG996_H */
