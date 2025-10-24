/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    delay_test.c
  * @brief   This file provides test code for the delay functions.
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
#include "log.h"

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
 * @brief 延时函数测试
 * @note 测试微秒和毫秒延时函数的准确性
 */
void Delay_Test(void)
{
    Log_Info("=== Delay Function Test (Optimized Version) ===");
    
    // 初始化延时系统
    Log_Info("Initializing delay system...");
    Delay_Init();
    Log_Info("Delay system initialized");
    
    // 测试微秒延时
    Log_Info("Testing Delay_Us function...");
    
    // 测试1微秒延时
    Log_Info("Testing 1us delay...");
    Delay_Us(1);
    Log_Info("1us delay completed");
    
    // 测试10微秒延时
    Log_Info("Testing 10us delay...");
    Delay_Us(10);
    Log_Info("10us delay completed");
    
    // 测试100微秒延时
    Log_Info("Testing 100us delay...");
    Delay_Us(100);
    Log_Info("100us delay completed");
    
    // 测试1000微秒延时
    Log_Info("Testing 1000us delay...");
    Delay_Us(1000);
    Log_Info("1000us delay completed");
    
    // 测试毫秒延时
    Log_Info("Testing Delay_Ms function...");
    
    // 测试1毫秒延时
    Log_Info("Testing 1ms delay...");
    Delay_Ms(1);
    Log_Info("1ms delay completed");
    
    // 测试10毫秒延时
    Log_Info("Testing 10ms delay...");
    Delay_Ms(10);
    Log_Info("10ms delay completed");
    
    Log_Info("=== Delay Function Test Completed ===");
}

/**
 * @brief 延时函数性能测试
 * @note 测试延时函数的实际延时时间
 */
void Delay_PerformanceTest(void)
{
    Log_Info("=== Delay Performance Test (Optimized Version) ===");
    
    // 确保延时系统已初始化
    Delay_Init();
    
    // 测试不同微秒延时的实际时间
    uint16_t test_values[] = {1, 10, 50, 100, 500, 1000, 5000, 10000};
    uint8_t test_count = sizeof(test_values) / sizeof(test_values[0]);
    
    for (uint8_t i = 0; i < test_count; i++) {
        Log_Info("Testing %dus delay...", test_values[i]);
        
        // 记录开始时间
        uint32_t start_time = HAL_GetTick();
        
        // 执行延时
        Delay_Us(test_values[i]);
        
        // 记录结束时间
        uint32_t end_time = HAL_GetTick();
        
        // 计算实际延时时间
        uint32_t actual_delay = end_time - start_time;
        
        Log_Info("Expected: %dus, Actual: %lums", test_values[i], actual_delay);
    }
    
    Log_Info("=== Delay Performance Test Completed ===");
}

/**
 * @brief 延时函数优化效果测试
 * @note 对比优化前后的性能差异
 */
void Delay_OptimizationTest(void)
{
    Log_Info("=== Delay Optimization Test ===");
    
    // 确保延时系统已初始化
    Delay_Init();
    
    // 测试连续调用性能
    Log_Info("Testing continuous delay calls...");
    
    uint32_t start_time = HAL_GetTick();
    
    // 连续调用1000次1微秒延时
    for (uint16_t i = 0; i < 1000; i++) {
        Delay_Us(1);
    }
    
    uint32_t end_time = HAL_GetTick();
    uint32_t total_time = end_time - start_time;
    
    Log_Info("1000 x 1us delays completed in %lums", total_time);
    Log_Info("Average overhead per call: %luus", (total_time * 1000) / 1000);
    
    // 测试不同延时时间的性能
    Log_Info("Testing various delay times...");
    
    uint16_t test_times[] = {1, 5, 10, 50, 100};
    uint8_t test_count = sizeof(test_times) / sizeof(test_times[0]);
    
    for (uint8_t i = 0; i < test_count; i++) {
        start_time = HAL_GetTick();
        
        // 连续调用100次指定延时
        for (uint8_t j = 0; j < 100; j++) {
            Delay_Us(test_times[i]);
        }
        
        end_time = HAL_GetTick();
        total_time = end_time - start_time;
        
        Log_Info("100 x %dus delays: %lums total", test_times[i], total_time);
    }
    
    Log_Info("=== Delay Optimization Test Completed ===");
}

/* USER CODE END EF */
