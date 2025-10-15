/**
  ******************************************************************************
  * @file    log_test.c
  * @brief   日志系统测试源文件
  * @author  AI Assistant
  * @date    2025
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "log_test.h"
#include "cmsis_os.h"

/* Private variables ---------------------------------------------------------*/
static osThreadId_t g_log_test_task_handle = NULL;

/* Private function prototypes -----------------------------------------------*/
static void LOG_TestTask(void *argument);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  日志系统测试任务
 * @param  argument: 任务参数
 * @retval None
 */
static void LOG_TestTask(void *argument)
{
    uint32_t test_counter = 0;
    
    LOG_INFO("Log test task started");
    
    while (1) {
        // Test different log levels
        LOG_ERROR("This is error log test #%lu", test_counter);
        osDelay(1000);
        
        LOG_WARNING("This is warning log test #%lu", test_counter);
        osDelay(1000);
        
        LOG_INFO("This is info log test #%lu", test_counter);
        osDelay(1000);
        
        LOG_DEBUG("This is debug log test #%lu", test_counter);
        osDelay(1000);
        
        // Test formatted output
        LOG_INFO("System status - Counter: %lu, Queue pending: %lu", 
                test_counter, LOG_GetPendingCount());
        osDelay(1000);
        
        // Test raw string output
        LOG_PrintRaw("=== Raw string test ===\r\n");
        osDelay(1000);
        
        // Test log level switching
        if (test_counter % 10 == 0) {
            LOG_INFO("Switch log level to WARNING");
            LOG_SetLevel(LOG_LEVEL_WARNING);
            osDelay(2000);
            
            LOG_INFO("Switch log level to DEBUG");
            LOG_SetLevel(LOG_LEVEL_DEBUG);
        }
        
        test_counter++;
        
        // Rest every 10 cycles
        if (test_counter % 10 == 0) {
            LOG_INFO("Test cycle completed %lu times, rest 5 seconds", test_counter);
            osDelay(5000);
        }
    }
}

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  Run log system test
 * @retval None
 */
void LOG_RunTest(void)
{
    // Create test task
    const osThreadAttr_t log_test_task_attributes = {
        .name = "LogTestTask",
        .stack_size = 256 * 4,
        .priority = (osPriority_t) osPriorityNormal,
    };
    
    g_log_test_task_handle = osThreadNew(LOG_TestTask, NULL, &log_test_task_attributes);
    
    if (g_log_test_task_handle != NULL) {
        LOG_INFO("Log test task created successfully");
    } else {
        LOG_ERROR("Log test task creation failed");
    }
}
