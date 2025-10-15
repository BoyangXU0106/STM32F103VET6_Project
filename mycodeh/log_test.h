/**
  ******************************************************************************
  * @file    log_test.h
  * @brief   日志系统测试头文件
  * @author  AI Assistant
  * @date    2025
  ******************************************************************************
  */

#ifndef __LOG_TEST_H
#define __LOG_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "log.h"

/* Private function prototypes -----------------------------------------------*/

/**
 * @brief  Log system test task
 * @param  argument: Task parameter
 * @retval None
 */
void LOG_TestTask(void *argument);

/**
 * @brief  Run log system test
 * @retval None
 */
void LOG_RunTest(void);

#ifdef __cplusplus
}
#endif

#endif /* __LOG_TEST_H */
