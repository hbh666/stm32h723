#ifndef ws2812status_H
#define ws2812status_H

/*---------------------INCLUDES----------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "tim.h"
#include "ws2812.h"

/*---------------------FUNCTIONS---------------------*/
void ws2812Task(void *argument);

void BlinkYellow(void);

void BlinkRed(void);

void BlinkBlue(void);

void BlinkGreen(void);

void BlinkWhite(void);

void BlinkCyan(void);

void BlinkPurple(void);

#endif
