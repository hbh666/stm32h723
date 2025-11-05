/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ws2812status.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for BeginTask */
osThreadId_t BeginTaskHandle;
const osThreadAttr_t BeginTask_attributes = {
  .name = "BeginTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for shoot */
osThreadId_t shootHandle;
const osThreadAttr_t shoot_attributes = {
  .name = "shoot",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for gimbal */
osThreadId_t gimbalHandle;
const osThreadAttr_t gimbal_attributes = {
  .name = "gimbal",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for chassis */
osThreadId_t chassisHandle;
const osThreadAttr_t chassis_attributes = {
  .name = "chassis",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for init */
osThreadId_t initHandle;
const osThreadAttr_t init_attributes = {
  .name = "init",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for vision */
osThreadId_t visionHandle;
const osThreadAttr_t vision_attributes = {
  .name = "vision",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for cmd */
osThreadId_t cmdHandle;
const osThreadAttr_t cmd_attributes = {
  .name = "cmd",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for reference */
osThreadId_t referenceHandle;
const osThreadAttr_t reference_attributes = {
  .name = "reference",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for WS2812Task */
osThreadId_t WS2812TaskHandle;
const osThreadAttr_t WS2812Task_attributes = {
  .name = "WS2812Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void ShootTask(void *argument);

void GimbalTask(void *argument);

void ChassisTask(void *argument);

void StartInitTask(void *argument);

void VisionTask(void *argument);

void CmdTask(void *argument);

void RefereeTask(void *argument);

void ws2812Task(void *argument);

extern void MX_USB_DEVICE_Init(void);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName) {
  /* Run time stack overflow checking is performed if
  configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
  called if a stack overflow is detected. */
}

/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
void vApplicationMallocFailedHook(void) {
  /* vApplicationMallocFailedHook() will only be called if
  configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
  function that will get called if a call to pvPortMalloc() fails.
  pvPortMalloc() is called internally by the kernel whenever a task, queue,
  timer or semaphore is created. It is also called by various parts of the
  demo application. If heap_1.c or heap_2.c are used, then the size of the
  heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
  FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
  to query the size of free heap space that remains (although it does not
  provide information on how the remaining heap might be fragmented). */
}

/* USER CODE END 5 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of BeginTask */
  BeginTaskHandle = osThreadNew(StartDefaultTask, NULL, &BeginTask_attributes);

  /* creation of shoot */
  shootHandle = osThreadNew(ShootTask, NULL, &shoot_attributes);

  /* creation of gimbal */
  gimbalHandle = osThreadNew(GimbalTask, NULL, &gimbal_attributes);

  /* creation of chassis */
  chassisHandle = osThreadNew(ChassisTask, NULL, &chassis_attributes);

  /* creation of init */
  initHandle = osThreadNew(StartInitTask, NULL, &init_attributes);

  /* creation of vision */
  visionHandle = osThreadNew(VisionTask, NULL, &vision_attributes);

  /* creation of cmd */
  cmdHandle = osThreadNew(CmdTask, NULL, &cmd_attributes);

  /* creation of reference */
  referenceHandle = osThreadNew(RefereeTask, NULL, &reference_attributes);

  /* creation of WS2812Task */
  WS2812TaskHandle = osThreadNew(ws2812Task, NULL, &WS2812Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the BeginTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
__weak void StartDefaultTask(void *argument) {
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for (;;) {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_ShootTask */
/**
* @brief Function implementing the shoot thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ShootTask */
__weak void ShootTask(void *argument) {
  /* USER CODE BEGIN ShootTask */
  /* Infinite loop */
  for (;;) {
    osDelay(1);
  }
  /* USER CODE END ShootTask */
}

/* USER CODE BEGIN Header_GimbalTask */
/**
* @brief Function implementing the gimbal thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_GimbalTask */
__weak void GimbalTask(void *argument) {
  /* USER CODE BEGIN GimbalTask */
  /* Infinite loop */
  for (;;) {
    osDelay(1);
  }
  /* USER CODE END GimbalTask */
}

/* USER CODE BEGIN Header_ChassisTask */
/**
* @brief Function implementing the chassis thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ChassisTask */
__weak void ChassisTask(void *argument) {
  /* USER CODE BEGIN ChassisTask */
  /* Infinite loop */
  for (;;) {
    osDelay(1);
  }
  /* USER CODE END ChassisTask */
}

/* USER CODE BEGIN Header_StartInitTask */
/**
* @brief Function implementing the init thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartInitTask */
__weak void StartInitTask(void *argument) {
  /* USER CODE BEGIN StartInitTask */
  /* Infinite loop */
  for (;;) {
    osDelay(1);
  }
  /* USER CODE END StartInitTask */
}

/* USER CODE BEGIN Header_VisionTask */
/**
* @brief Function implementing the vision thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_VisionTask */
__weak void VisionTask(void *argument) {
  /* USER CODE BEGIN VisionTask */
  /* Infinite loop */
  for (;;) {
    osDelay(1);
  }
  /* USER CODE END VisionTask */
}

/* USER CODE BEGIN Header_CmdTask */
/**
* @brief Function implementing the cmd thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_CmdTask */
__weak void CmdTask(void *argument) {
  /* USER CODE BEGIN CmdTask */
  /* Infinite loop */
  for (;;) {
    osDelay(1);
  }
  /* USER CODE END CmdTask */
}

/* USER CODE BEGIN Header_RefereeTask */
/**
* @brief Function implementing the reference thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_RefereeTask */
__weak void RefereeTask(void *argument) {
  /* USER CODE BEGIN RefereeTask */
  /* Infinite loop */
  for (;;) {
    osDelay(1);
  }
  /* USER CODE END RefereeTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

