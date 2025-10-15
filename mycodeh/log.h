/**
  ******************************************************************************
  * @file    log.h
  * @brief   日志系统头文件
  * @author  AI Assistant
  * @date    2025
  ******************************************************************************
  * @attention
  * 本日志系统使用DMA+队列+FreeRTOS任务的形式，确保日志信息不会丢失
  ******************************************************************************
  */

#ifndef __LOG_H
#define __LOG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "cmsis_os.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define LOG_MAX_MESSAGE_LENGTH    256     // Maximum length of single log message
#define LOG_QUEUE_SIZE           32      // Log queue size
#define LOG_TASK_STACK_SIZE     1024     // 增加到1KB栈空间，DMA传输需要更多栈
#define LOG_TASK_PRIORITY       osPriorityHigh  // Log task priority

/* Log level definitions */
typedef enum {
    LOG_LEVEL_ERROR = 0,     // Error level
    LOG_LEVEL_WARNING,       // Warning level
    LOG_LEVEL_INFO,          // Info level
    LOG_LEVEL_DEBUG,         // Debug level
    LOG_LEVEL_MAX
} log_level_t;

/* Log message structure */
typedef struct {
    uint32_t timestamp;      // Timestamp (milliseconds)
    log_level_t level;       // Log level
    char message[LOG_MAX_MESSAGE_LENGTH];  // Log message content
} log_message_t;

/* Log system status */
typedef enum {
    LOG_STATUS_UNINITIALIZED = 0,  // Uninitialized
    LOG_STATUS_INITIALIZED,        // Initialized
    LOG_STATUS_RUNNING,            // Running
    LOG_STATUS_ERROR               // Error status
} log_status_t;

/* Private function prototypes -----------------------------------------------*/

/**
 * @brief  Initialize log system
 * @retval HAL status
 */
HAL_StatusTypeDef LOG_Init(void);

/**
 * @brief  Deinitialize log system
 * @retval HAL status
 */
HAL_StatusTypeDef LOG_DeInit(void);

/**
 * @brief  Get log system status
 * @retval Log system status
 */
log_status_t LOG_GetStatus(void);

/**
 * @brief  Print error level log
 * @param  format: Format string
 * @param  ...: Variable arguments
 * @retval None
 */
void LOG_ERROR(const char* format, ...);

/**
 * @brief  Print warning level log
 * @param  format: Format string
 * @param  ...: Variable arguments
 * @retval None
 */
void LOG_WARNING(const char* format, ...);

/**
 * @brief  Print info level log
 * @param  format: Format string
 * @param  ...: Variable arguments
 * @retval None
 */
void LOG_INFO(const char* format, ...);

/**
 * @brief  Print debug level log
 * @param  format: Format string
 * @param  ...: Variable arguments
 * @retval None
 */
void LOG_DEBUG(const char* format, ...);

/**
 * @brief  Print log with specified level
 * @param  level: Log level
 * @param  format: Format string
 * @param  ...: Variable arguments
 * @retval None
 */
void LOG_Print(log_level_t level, const char* format, ...);

/**
 * @brief  Print raw string (without timestamp and level)
 * @param  str: String to print
 * @retval None
 */
void LOG_PrintRaw(const char* str);

/**
 * @brief  Set log level filtering
 * @param  level: Minimum log level
 * @retval None
 */
void LOG_SetLevel(log_level_t level);

/**
 * @brief  Get current log level
 * @retval Current log level
 */
log_level_t LOG_GetLevel(void);

/**
 * @brief  Get number of pending messages in queue
 * @retval Number of pending messages
 */
uint32_t LOG_GetPendingCount(void);

/**
 * @brief  Clear log queue
 * @retval None
 */
void LOG_ClearQueue(void);

/* Log level string mapping */
extern const char* log_level_strings[LOG_LEVEL_MAX];

#ifdef __cplusplus
}
#endif

#endif /* __LOG_H */
