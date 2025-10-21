/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    log.c
  * @brief   日志系统源文件
  *          基于串口1DMA+队列+日志任务的日志功能实现
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
#include "log.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* 外部变量声明 */
extern osMessageQueueId_t logQueueHandle;
extern UART_HandleTypeDef huart1;

/* 私有变量 */
static LogLevel_t current_log_level = LOG_LEVEL_DEBUG;  /* 当前日志级别 */
static const char* log_level_strings[LOG_LEVEL_MAX] = {
    "ERROR",
    "WARN ",
    "INFO ",
    "DEBUG"
};

/* USER CODE BEGIN Private Variables */

/* USER CODE END Private Variables */

/* 私有函数声明 */
static void Log_SendMessage(LogLevel_t level, const char* message);

/* USER CODE BEGIN Private Function Prototypes */

/* USER CODE END Private Function Prototypes */

/**
  * @brief  日志系统初始化
  * @param  None
  * @retval None
  */
void Log_Init(void)
{
    /* 初始化日志系统 */
    current_log_level = LOG_LEVEL_DEBUG;
    
    /* 发送启动信息 */
    Log_Info("log init success");
}

/**
  * @brief  获取系统时间戳
  * @param  None
  * @retval 时间戳(毫秒)
  */
uint32_t Log_GetTimestamp(void)
{
    return osKernelGetTickCount();
}

/**
  * @brief  发送日志消息到队列
  * @param  level: 日志级别
  * @param  message: 日志消息
  * @retval None
  */
static void Log_SendMessage(LogLevel_t level, const char* message)
{
    LogMessage_t log_msg;
    
    /* 检查日志级别 */
    if (level > current_log_level) {
        return;
    }
    
    /* 填充日志消息结构体 */
    log_msg.level = level;
    log_msg.timestamp = Log_GetTimestamp();
    strncpy(log_msg.message, message, sizeof(log_msg.message) - 1);
    log_msg.message[sizeof(log_msg.message) - 1] = '\0';
    
    /* 发送到队列 */
    if (osMessageQueuePut(logQueueHandle, &log_msg, 0, 0) != osOK) {
        /* 队列满，丢弃消息 */
    }
}

/**
  * @brief  通用日志输出函数
  * @param  level: 日志级别
  * @param  format: 格式化字符串
  * @param  ...: 可变参数
  * @retval None
  */
void Log_Print(LogLevel_t level, const char* format, ...)
{
    char buffer[56];
    va_list args;
    
    /* 检查日志级别 */
    if (level > current_log_level) {
        return;
    }
    
    /* 格式化消息 */
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    /* 发送消息 */
    Log_SendMessage(level, buffer);
}

/**
  * @brief  错误级别日志输出
  * @param  format: 格式化字符串
  * @param  ...: 可变参数
  * @retval None
  */
void Log_Error(const char* format, ...)
{
    char buffer[56];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Log_SendMessage(LOG_LEVEL_ERROR, buffer);
}

/**
  * @brief  警告级别日志输出
  * @param  format: 格式化字符串
  * @param  ...: 可变参数
  * @retval None
  */
void Log_Warn(const char* format, ...)
{
    char buffer[56];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Log_SendMessage(LOG_LEVEL_WARN, buffer);
}

/**
  * @brief  信息级别日志输出
  * @param  format: 格式化字符串
  * @param  ...: 可变参数
  * @retval None
  */
void Log_Info(const char* format, ...)
{
    char buffer[56];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Log_SendMessage(LOG_LEVEL_INFO, buffer);
}

/**
  * @brief  调试级别日志输出
  * @param  format: 格式化字符串
  * @param  ...: 可变参数
  * @retval None
  */
void Log_Debug(const char* format, ...)
{
    char buffer[56];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Log_SendMessage(LOG_LEVEL_DEBUG, buffer);
}

/**
  * @brief  设置日志级别
  * @param  level: 日志级别
  * @retval None
  */
void Log_SetLevel(LogLevel_t level)
{
    if (level < LOG_LEVEL_MAX) {
        current_log_level = level;
    }
}

/**
  * @brief  获取当前日志级别
  * @param  None
  * @retval 当前日志级别
  */
LogLevel_t Log_GetLevel(void)
{
    return current_log_level;
}

/* USER CODE BEGIN Application */

/* USER CODE END Application */
