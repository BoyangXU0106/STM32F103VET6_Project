/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    delay.c
  * @brief   This file provides code for the delay functions.
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
#include "delay.h"
#include "tim.h"

/* USER CODE BEGIN Includes */

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
/* USER CODE BEGIN PV */
static uint8_t timer_initialized = 0;  // 定时器初始化标志
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static void Delay_InitTimer(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief 初始化延时定时器
 * @note 确保TIM3持续运行，避免每次调用都启停定时器
 */
static void Delay_InitTimer(void)
{
    if (!timer_initialized) {
        // 启动TIM3并保持运行状态
        HAL_TIM_Base_Start(&htim3);
        timer_initialized = 1;
    }
}

/* USER CODE END 0 */

/* Exported functions --------------------------------------------------------*/
/* USER CODE BEGIN EF */

/**
 * @brief 微秒级延时函数（优化版本）
 * @param us 延时时间（微秒），范围：1-65535
 * @note 基于持续运行的TIM3实现，精度为1微秒
 * @note 在单总线协议等需要精确时序的场合使用
 * 
 * 优化特点：
 * - 定时器持续运行，避免启停开销
 * - 更简洁的实现逻辑
 * - 更好的性能表现
 * 
 * 实现原理：
 * - TIM3时钟频率 = 1MHz（APB1=36MHz，预分频器=35）
 * - 1个计数周期 = 1微秒
 * - 通过计算时间差实现精确延时
 */
void Delay_Us(uint16_t us)
{
    // 参数检查
    if (us == 0) return;
    
    // 确保定时器已初始化并持续运行
    Delay_InitTimer();
    
    // 获取起始时间
    uint16_t start_time = __HAL_TIM_GET_COUNTER(&htim3);
    uint16_t current_time;
    uint16_t elapsed;
    
    // 等待延时完成
    do {
        current_time = __HAL_TIM_GET_COUNTER(&htim3);
        
        // 计算已过去的时间（处理溢出情况）
        if (current_time >= start_time) {
            // 正常情况：current_time - start_time
            elapsed = current_time - start_time;
        } else {
            // 溢出情况：(65535 - start_time) + current_time + 1
            elapsed = (65535 - start_time) + current_time + 1;
        }
        
        // 检查是否达到目标延时时间
        if (elapsed >= us) {
            break;
        }
    } while (1);
}

/**
 * @brief 毫秒级延时函数
 * @param ms 延时时间（毫秒），范围：1-65535
 * @note 基于HAL_Delay实现，用于一般延时
 */
void Delay_Ms(uint16_t ms)
{
    // 参数检查
    if (ms == 0) return;
    
    // 使用HAL库的毫秒延时函数
    HAL_Delay(ms);
}

/**
 * @brief 延时系统初始化
 * @note 在系统启动时调用，启动TIM3定时器
 * @note 建议在main函数中调用，确保延时函数正常工作
 */
void Delay_Init(void)
{
    // 启动TIM3定时器并保持运行状态
    Delay_InitTimer();
}

/* USER CODE END EF */
