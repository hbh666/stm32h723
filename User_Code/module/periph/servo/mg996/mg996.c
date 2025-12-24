/**
    * @file mg996.c
    * @author TuxMonkey (https://github.com/TuxMonkey)
    * @brief MG996 servo motor driver (MG996舵机驱动)
    * @version 1.0
    * @date 2024-06-10
**/

#include <stdlib.h>
#include <string.h>
#include "mg996.h"
#include "bsp_pwm.h"


MG996Instance *MG996Register(MG996_Init_Config_s *config)
{
    MG996Instance *mg996 = (MG996Instance *) malloc(sizeof(MG996Instance));
    memset(mg996, 0, sizeof(MG996Instance));

    mg996->pwm = PWMRegister(config->pwm_config);
    mg996->min_dutyratio = config->min_dutyratio;
    mg996->max_dutyratio = config->max_dutyratio;
    mg996->min_angle = config->min_angle;
    mg996->max_angle = config->max_angle;

    return mg996;
}

void MG996SetAngle(MG996Instance *mg996, float angle)
{
    if (angle < mg996->min_angle)
        angle = mg996->min_angle;
    else if (angle > mg996->max_angle)
        angle = mg996->max_angle;

    auto ratio = (angle - mg996->min_angle) / (mg996->max_angle - mg996->min_angle);
    auto dutyratio = mg996->min_dutyratio + ratio * (mg996->max_dutyratio - mg996->min_dutyratio);

    PWMSetDutyRatio(mg996->pwm, dutyratio);
}
