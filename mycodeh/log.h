/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    log.h
  * @brief   日志系统头文件
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

#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "usart.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* 日志级别定义 */
typedef enum {
    LOG_LEVEL_ERROR = 0,    /* 错误级别 */
    LOG_LEVEL_WARN,         /* 警告级别 */
    LOG_LEVEL_INFO,         /* 信息级别 */
    LOG_LEVEL_DEBUG,        /* 调试级别 */
    LOG_LEVEL_MAX
} LogLevel_t;

/* 日志消息结构体 */
typedef struct {
    LogLevel_t level;       /* 日志级别 */
    uint32_t timestamp;     /* 时间戳 */
    char message[56];       /* 日志消息内容(64-8=56字节) */
} LogMessage_t;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

/* 日志系统初始化 */
void Log_Init(void);

/* 日志输出函数 */
void Log_Error(const char* format, ...);
void Log_Warn(const char* format, ...);
void Log_Info(const char* format, ...);
void Log_Debug(const char* format, ...);

/* 通用日志输出函数 */
void Log_Print(LogLevel_t level, const char* format, ...);

/* 获取时间戳 */
uint32_t Log_GetTimestamp(void);

/* 日志级别设置 */
void Log_SetLevel(LogLevel_t level);
LogLevel_t Log_GetLevel(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __LOG_H__ */
