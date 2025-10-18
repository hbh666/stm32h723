/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ACC_CS_Pin GPIO_PIN_0
#define ACC_CS_GPIO_Port GPIOC
#define GYRO_CS_Pin GPIO_PIN_3
#define GYRO_CS_GPIO_Port GPIOC
#define pump1_Pin GPIO_PIN_0
#define pump1_GPIO_Port GPIOA
#define pump2_Pin GPIO_PIN_2
#define pump2_GPIO_Port GPIOA
#define pump3_Pin GPIO_PIN_9
#define pump3_GPIO_Port GPIOE
#define ACC_INT_Pin GPIO_PIN_10
#define ACC_INT_GPIO_Port GPIOE
#define GYRO_INT_Pin GPIO_PIN_12
#define GYRO_INT_GPIO_Port GPIOE
#define pump4_Pin GPIO_PIN_13
#define pump4_GPIO_Port GPIOE
#define cs2_Pin GPIO_PIN_15
#define cs2_GPIO_Port GPIOE
#define cs1_Pin GPIO_PIN_10
#define cs1_GPIO_Port GPIOB
#define cs4_Pin GPIO_PIN_11
#define cs4_GPIO_Port GPIOB
#define cs3_Pin GPIO_PIN_10
#define cs3_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
