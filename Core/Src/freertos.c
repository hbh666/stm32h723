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
#include "ins_task.h"
#include "buzzer.h"
// #include "remoteTask.h"
//
// #include "bsp_dwt.h"
// // #include "ins_task.h"
// #include "bsp_log.h"
#include "dev_cmd.h"
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
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
//@Todo:测试instask
const osThreadAttr_t instask_attributes = {
  .name = "instask",
  .priority = osPriorityAboveNormal, // 较高优先级
  .stack_size = 1024 * 4 // 栈大小，单位是字节（通常是字数的4倍）
};

//@Todo:测试使用
osThreadId insTaskHandle;

void StartINSTASK(void const *argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void ShootTask(void *argument);

void GimbalTask(void *argument);

void ChassisTask(void *argument);

void StartInitTask(void *argument);

void VisionTask(void *argument);

void CmdTask(void *argument);

void RefereeTask(void *argument);

extern void ws2812Task(void *argument);

extern void MX_USB_DEVICE_Init(void);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
  /* Run time stack overflow checking is performed if
  configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
  called if a stack overflow is detected. */
}

/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
void vApplicationMallocFailedHook(void)
{
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
void MX_FREERTOS_Init(void)
{
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
  //@Todo:INS_Task是测试版本
  // 创建线程
  insTaskHandle = osThreadNew(StartINSTASK, NULL, &instask_attributes);
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
__weak void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for (;;)
  {
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
__weak void ShootTask(void *argument)
{
  /* USER CODE BEGIN ShootTask */
  /* Infinite loop */
  for (;;)
  {
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
__weak void GimbalTask(void *argument)
{
  /* USER CODE BEGIN GimbalTask */
  /* Infinite loop */
  for (;;)
  {
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
__weak void ChassisTask(void *argument)
{
  /* USER CODE BEGIN ChassisTask */
  /* Infinite loop */
  for (;;)
  {
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
__weak void StartInitTask(void *argument)
{
  /* USER CODE BEGIN StartInitTask */
  /* Infinite loop */
  for (;;)
  {
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
__weak void VisionTask(void *argument)
{
  /* USER CODE BEGIN VisionTask */
  /* Infinite loop */
  for (;;)
  {
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
__weak void CmdTask(void *argument)
{
  /* USER CODE BEGIN CmdTask */
  /* Infinite loop */
  for (;;)
  {
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
__weak void RefereeTask(void *argument)
{
  /* USER CODE BEGIN RefereeTask */
  /* Infinite loop */
  for (;;)
  {
    osDelay(1);
  }
  /* USER CODE END RefereeTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

//@Todo:INS_Task是测试阶段使用。
__attribute__((noreturn)) void StartINSTASK(void const *argument)
{
  static float ins_start;
  static float ins_dt;
  INS_Init(); // 确保BMI088被正确初始化.
  // LOGINFO("[freeRTOS] INS Task Start");
  for (;;)
  {
    // 1kHz
    // ins_start = DWT_GetTimeline_ms();
    INS_Task();
    // ins_dt = DWT_GetTimeline_ms() - ins_start;
    // if (ins_dt > 1)
    // @Todo: LOGERROR("[freeRTOS] INS Task is being DELAY! dt = [%f]", &ins_dt);
    // // VisionSend(); // 解算完成后发送视觉数据,但是当前的实现不太优雅,后续若添加硬件触发需要重新考虑结构的组织
    osDelay(1); // 1ms, 1kHz
  }
}


/* USER CODE END Application */

