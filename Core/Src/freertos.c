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
#include "log.h"
#include "log_test.h"
#include "app_lvgl.h"
#include "lvgl.h"
#include "gui_guider.h"
#include "pressure_task.h"
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
osThreadId_t lvglTaskHandle;
const osThreadAttr_t app_lvgl_Task_attributes = {
  .name = "app_lvgl_Task",
  .stack_size = 2048 * 4,  // 增加到8KB栈空间，LVGL需要更多栈空间
  .priority = (osPriority_t) osPriorityHigh,  // 提高优先级确保UI响应
};

// 压力任务句柄
osThreadId_t pressureTaskHandle;
// 压力任务属性
const osThreadAttr_t pressureTask_attributes = {
    .name = "pressureTask",
    .stack_size = 512 * 4,  // 减少到2KB栈空间，压力采集任务较简单
    .priority = (osPriority_t) osPriorityNormal,
};

/* USER CODE END Variables */
/* Definitions for defaultTask */

osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 256 * 4,  // 减少到512字节，默认任务很简单
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  // 初始化日志系统
  if(LOG_Init() != HAL_OK){
  };
	
  app_lvgl_init();
  
  // 初始化压力任务
  pressure_task_init();
	
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
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
	
  lvglTaskHandle = osThreadNew(app_lvgl_task, NULL, &app_lvgl_Task_attributes);
  
  // 创建压力采集任务
  pressureTaskHandle = osThreadNew(pressure_task, NULL, &pressureTask_attributes);
  if (pressureTaskHandle == NULL) {
      LOG_ERROR("Failed to create pressure task!");
  } else {
      LOG_INFO("Pressure task created successfully - Handle: %p", pressureTaskHandle);
  }
  
  // Start log test task
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  // 等待一小段时间确保日志系统完全初始化
  osDelay(3000);
  
  // 测试日志系统
  LOG_INFO("log init success");
  LOG_DEBUG("default task start");
  
  // 添加栈使用监控
  UBaseType_t stack_high_water_mark;
  stack_high_water_mark = uxTaskGetStackHighWaterMark(NULL);
  LOG_DEBUG("Default task stack high water mark: %lu", stack_high_water_mark);
  
  int counter = 0;
	
  /* Infinite loop */
  for(;;)
  {
    // 每5秒打印一次心跳日志
    LOG_DEBUG("log counter: %lu", counter);
		counter ++;
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

