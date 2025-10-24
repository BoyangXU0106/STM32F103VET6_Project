/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    delay.h
  * @brief   This file contains the headers of the delay functions.
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
#ifndef __DELAY_H
#define __DELAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

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
/* USER CODE BEGIN EFP */

/**
 * @brief 延时系统初始化
 * @note 在系统启动时调用，启动TIM3定时器
 * @note 建议在main函数中调用，确保延时函数正常工作
 */
void Delay_Init(void);

/**
 * @brief 微秒级延时函数（优化版本）
 * @param us 延时时间（微秒），范围：1-65535
 * @note 基于持续运行的TIM3实现，精度为1微秒
 * @note 在单总线协议等需要精确时序的场合使用
 * @note 优化特点：定时器持续运行，避免启停开销
 */
void Delay_Us(uint16_t us);

/**
 * @brief 毫秒级延时函数
 * @param ms 延时时间（毫秒），范围：1-65535
 * @note 基于HAL_Delay实现，用于一般延时
 */
void Delay_Ms(uint16_t ms);

/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_H */
