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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* Exported functions --------------------------------------------------------*/
/* USER CODE BEGIN EF */

/**
 * @brief 微秒级延时函数
 * @param us 延时时间（微秒），范围：1-65535
 * @note 基于TIM3实现，精度为1微秒
 * @note 在单总线协议等需要精确时序的场合使用
 * 
 * 实现原理：
 * - TIM3时钟频率 = 1MHz（APB1=36MHz，预分频器=35）
 * - 1个计数周期 = 1微秒
 * - 通过读取TIM3计数器值实现精确延时
 */
void Delay_Us(uint16_t us)
{
    // 参数检查
    if (us == 0) return;
    
    // 启动TIM3
    HAL_TIM_Base_Start(&htim3);
    
    // 获取当前计数器值
    uint16_t start_time = __HAL_TIM_GET_COUNTER(&htim3);
    uint16_t current_time;
    
    // 等待延时完成
    do {
        current_time = __HAL_TIM_GET_COUNTER(&htim3);
        
        // 处理计数器溢出情况
        if (current_time < start_time) {
            // 计数器溢出，计算剩余延时时间
            uint16_t elapsed = (65535 - start_time) + current_time + 1;
            if (elapsed >= us) {
                break;
            }
            us -= elapsed;
            start_time = current_time;
        } else {
            // 正常情况
            if ((current_time - start_time) >= us) {
                break;
            }
        }
    } while (1);
    
    // 停止TIM3
    HAL_TIM_Base_Stop(&htim3);
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

/* USER CODE END EF */
