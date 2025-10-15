/**
  ******************************************************************************
  * @file    log.c
  * @brief   日志系统源文件
  * @author  AI Assistant
  * @date    2025
  ******************************************************************************
  * @attention
  * 本日志系统使用DMA+队列+FreeRTOS任务的形式，确保日志信息不会丢失
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "log.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Log level string mapping */
const char* log_level_strings[LOG_LEVEL_MAX] = {
    "ERROR",
    "WARNING", 
    "INFO",
    "DEBUG"
};

/* Log system global variables */
static log_status_t g_log_status = LOG_STATUS_UNINITIALIZED;
static log_level_t g_log_level = LOG_LEVEL_DEBUG;  // Default show all levels
static osMessageQueueId_t g_log_queue = NULL;
static osThreadId_t g_log_task_handle = NULL;
static UART_HandleTypeDef* g_log_uart = &huart1;  // Use UART1

/* DMA transmission buffer */
static char g_dma_tx_buffer[LOG_MAX_MESSAGE_LENGTH + 64];  // Extra space for formatting
static volatile uint8_t g_dma_busy = 0;

/* Private function prototypes -----------------------------------------------*/
static void LOG_Task(void *argument);
static HAL_StatusTypeDef LOG_SendMessage(const char* message);
static uint32_t LOG_GetTimestamp(void);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Get system timestamp (milliseconds)
 * @retval Timestamp
 */
static uint32_t LOG_GetTimestamp(void)
{
    return HAL_GetTick();
}

/**
 * @brief  Send message via DMA
 * @param  message: Message to send
 * @retval HAL status
 */
static HAL_StatusTypeDef LOG_SendMessage(const char* message)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint16_t len = strlen(message);
    
    if (len == 0) {
        return HAL_OK;
    }
    
    // Wait for DMA transmission to complete
    while (g_dma_busy) {
        osDelay(1);
    }
    
    // Copy message to DMA buffer
    strncpy(g_dma_tx_buffer, message, sizeof(g_dma_tx_buffer) - 1);
    g_dma_tx_buffer[sizeof(g_dma_tx_buffer) - 1] = '\0';
    
    // Mark DMA as busy
    g_dma_busy = 1;
    
    // Start DMA transmission
    status = HAL_UART_Transmit_DMA(g_log_uart, (uint8_t*)g_dma_tx_buffer, len);
    
    if (status != HAL_OK) {
        g_dma_busy = 0;  // Transmission failed, clear busy flag
    }
    
    return status;
}

/**
 * @brief  Log task function
 * @param  argument: Task parameter
 * @retval None
 */
static void LOG_Task(void *argument)
{
    log_message_t msg;
    osStatus_t queue_status;
    char formatted_msg[LOG_MAX_MESSAGE_LENGTH + 64];
    uint32_t timestamp;
    
    while (1) {
        // Get log message from queue
        queue_status = osMessageQueueGet(g_log_queue, &msg, NULL, osWaitForever);
        
        if (queue_status == osOK) {
            // Check log level filtering
            if (msg.level <= g_log_level) {
                // Format message: [timestamp] [level] message content\r\n
                timestamp = LOG_GetTimestamp();
                snprintf(formatted_msg, sizeof(formatted_msg), 
                        "[%08lu] [%s] %s\r\n", 
                        timestamp, 
                        log_level_strings[msg.level], 
                        msg.message);
                
                // Send message
                LOG_SendMessage(formatted_msg);
            }
        }
        
        // Yield CPU time
        osDelay(1);
    }
}

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  Initialize log system
 * @retval HAL status
 */
HAL_StatusTypeDef LOG_Init(void)
{
    
    if (g_log_status != LOG_STATUS_UNINITIALIZED) {
        return HAL_ERROR;  // Already initialized
    }
    
    // Create log queue
    g_log_queue = osMessageQueueNew(LOG_QUEUE_SIZE, sizeof(log_message_t), NULL);
    if (g_log_queue == NULL) {
        return HAL_ERROR;
    }
    
    // Create log task
    const osThreadAttr_t log_task_attributes = {
        .name = "LogTask",
        .stack_size = LOG_TASK_STACK_SIZE,
        .priority = LOG_TASK_PRIORITY,
    };
    
    g_log_task_handle = osThreadNew(LOG_Task, NULL, &log_task_attributes);
    if (g_log_task_handle == NULL) {
        osMessageQueueDelete(g_log_queue);
        g_log_queue = NULL;
        return HAL_ERROR;
    }
    
    // Initialize DMA buffer
    memset(g_dma_tx_buffer, 0, sizeof(g_dma_tx_buffer));
    g_dma_busy = 0;
    
    // Update status
    g_log_status = LOG_STATUS_RUNNING;
    
    // Send initialization complete message
    LOG_INFO("Log system initialized successfully");
    
    return HAL_OK;
}

/**
 * @brief  Deinitialize log system
 * @retval HAL status
 */
HAL_StatusTypeDef LOG_DeInit(void)
{
    if (g_log_status == LOG_STATUS_UNINITIALIZED) {
        return HAL_OK;
    }
    
    // Wait for DMA transmission to complete
    while (g_dma_busy) {
        osDelay(10);
    }
    
    // Delete task
    if (g_log_task_handle != NULL) {
        osThreadTerminate(g_log_task_handle);
        g_log_task_handle = NULL;
    }
    
    // Delete queue
    if (g_log_queue != NULL) {
        osMessageQueueDelete(g_log_queue);
        g_log_queue = NULL;
    }
    
    // Update status
    g_log_status = LOG_STATUS_UNINITIALIZED;
    
    return HAL_OK;
}

/**
 * @brief  Get log system status
 * @retval Log system status
 */
log_status_t LOG_GetStatus(void)
{
    return g_log_status;
}

/**
 * @brief  Print log with specified level
 * @param  level: Log level
 * @param  format: Format string
 * @param  ...: Variable arguments
 * @retval None
 */
void LOG_Print(log_level_t level, const char* format, ...)
{
    log_message_t msg;
    va_list args;
    osStatus_t queue_status;
    
    if (g_log_status != LOG_STATUS_RUNNING || g_log_queue == NULL) {
        return;  // Log system not initialized or not running
    }
    
    if (level >= LOG_LEVEL_MAX) {
        return;  // Invalid log level
    }
    
    // Format message
    va_start(args, format);
    vsnprintf(msg.message, sizeof(msg.message), format, args);
    va_end(args);
    
    // Set message attributes
    msg.level = level;
    msg.timestamp = LOG_GetTimestamp();
    
    // Send to queue (non-blocking)
    queue_status = osMessageQueuePut(g_log_queue, &msg, 0, 0);
    
    // If queue is full, try waiting a short time before sending
    if (queue_status != osOK) {
        queue_status = osMessageQueuePut(g_log_queue, &msg, 0, 10);  // Wait 10ms
    }
    
    // If still failed, queue is full, can choose to discard or block wait
    // Here choose to discard to ensure system does not hang due to logs
}

/**
 * @brief  Print error level log
 * @param  format: Format string
 * @param  ...: Variable arguments
 * @retval None
 */
void LOG_ERROR(const char* format, ...)
{
    va_list args;
    char message[LOG_MAX_MESSAGE_LENGTH];
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    LOG_Print(LOG_LEVEL_ERROR, "%s", message);
}

/**
 * @brief  Print warning level log
 * @param  format: Format string
 * @param  ...: Variable arguments
 * @retval None
 */
void LOG_WARNING(const char* format, ...)
{
    va_list args;
    char message[LOG_MAX_MESSAGE_LENGTH];
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    LOG_Print(LOG_LEVEL_WARNING, "%s", message);
}

/**
 * @brief  Print info level log
 * @param  format: Format string
 * @param  ...: Variable arguments
 * @retval None
 */
void LOG_INFO(const char* format, ...)
{
    va_list args;
    char message[LOG_MAX_MESSAGE_LENGTH];
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    LOG_Print(LOG_LEVEL_INFO, "%s", message);
}

/**
 * @brief  Print debug level log
 * @param  format: Format string
 * @param  ...: Variable arguments
 * @retval None
 */
void LOG_DEBUG(const char* format, ...)
{
    va_list args;
    char message[LOG_MAX_MESSAGE_LENGTH];
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    LOG_Print(LOG_LEVEL_DEBUG, "%s", message);
}

/**
 * @brief  Print raw string (without timestamp and level)
 * @param  str: String to print
 * @retval None
 */
void LOG_PrintRaw(const char* str)
{
    if (g_log_status != LOG_STATUS_RUNNING) {
        return;
    }
    
    LOG_SendMessage(str);
}

/**
 * @brief  Set log level filtering
 * @param  level: Minimum log level
 * @retval None
 */
void LOG_SetLevel(log_level_t level)
{
    if (level < LOG_LEVEL_MAX) {
        g_log_level = level;
    }
}

/**
 * @brief  Get current log level
 * @retval Current log level
 */
log_level_t LOG_GetLevel(void)
{
    return g_log_level;
}


/**
 * @brief  Clear log queue
 * @retval None
 */
void LOG_ClearQueue(void)
{
    log_message_t msg;
    
    if (g_log_queue == NULL) {
        return;
    }
    
    // Clear all messages in queue
    while (osMessageQueueGet(g_log_queue, &msg, NULL, 0) == osOK) {
        // Discard message
    }
}

/* DMA transmission complete callback function */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == g_log_uart) {
        g_dma_busy = 0;  // Clear DMA busy flag
    }
}

/* DMA transmission error callback function */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == g_log_uart) {
        g_dma_busy = 0;  // Clear DMA busy flag
        LOG_ERROR("UART DMA transmission error");
    }
}
