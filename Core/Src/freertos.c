/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "log.h"
#include "ble_data.h"
#include "modbus.h"
#include "delay.h"
#include "usart.h"
#include "bsp_ili9341_lcd.h"
#include "bsp_xpt2046_lcd.h"
#include "pressure.h"
#include <string.h>
#include "i2c.h"
#include "bsp_dht11.h"
#include "tim.h"
#include "bsp_dwt.h"
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
/* USER CODE BEGIN Variables */
/* BLE接收缓冲区 */
  uint8_t bt_rx_buffer[256];
/* 全局任务句柄 */
osThreadId_t g_PressureTaskHandle;  // 压力任务全局句柄
osThreadId_t g_LCDTaskHandle;       // LCD任务全局句柄
osThreadId_t g_DHT11TaskHandle;  // dht11任务全局句柄
/* USER CODE END Variables */
/* Definitions for Log_Task */
osThreadId_t Log_TaskHandle;
const osThreadAttr_t Log_Task_attributes = {
  .name = "Log_Task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Pressure_Task */
osThreadId_t Pressure_TaskHandle;
const osThreadAttr_t Pressure_Task_attributes = {
  .name = "Pressure_Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow1,
};
/* Definitions for Usart2_Task */
osThreadId_t Usart2_TaskHandle;
const osThreadAttr_t Usart2_Task_attributes = {
  .name = "Usart2_Task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow1,
};
/* Definitions for LCD_Task */
osThreadId_t LCD_TaskHandle;
const osThreadAttr_t LCD_Task_attributes = {
  .name = "LCD_Task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Monitor_Task */
osThreadId_t Monitor_TaskHandle;
const osThreadAttr_t Monitor_Task_attributes = {
  .name = "Monitor_Task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for BLE_Task */
osThreadId_t BLE_TaskHandle;
const osThreadAttr_t BLE_Task_attributes = {
  .name = "BLE_Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for DHT11_Task */
osThreadId_t DHT11_TaskHandle;
const osThreadAttr_t DHT11_Task_attributes = {
  .name = "DHT11_Task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for FLASH_Task */
osThreadId_t FLASH_TaskHandle;
const osThreadAttr_t FLASH_Task_attributes = {
  .name = "FLASH_Task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for logQueue */
osMessageQueueId_t logQueueHandle;
const osMessageQueueAttr_t logQueue_attributes = {
  .name = "logQueue"
};
/* Definitions for BLEQueue */
osMessageQueueId_t BLEQueueHandle;
const osMessageQueueAttr_t BLEQueue_attributes = {
  .name = "BLEQueue"
};
/* Definitions for uart1_mutex */
osMutexId_t uart1_mutexHandle;
const osMutexAttr_t uart1_mutex_attributes = {
  .name = "uart1_mutex"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void LogTask(void *argument);
void PressureTask(void *argument);
void Usart2Task(void *argument);
void LCDTask(void *argument);
void MonitorTask(void *argument);
void BLETask(void *argument);
void DHT11Task(void *argument);
void FLASHTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of uart1_mutex */
  uart1_mutexHandle = osMutexNew(&uart1_mutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */

  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of logQueue */
  logQueueHandle = osMessageQueueNew (10, 64, &logQueue_attributes);

  /* creation of BLEQueue */
  BLEQueueHandle = osMessageQueueNew (5, 262, &BLEQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Log_Task */
  Log_TaskHandle = osThreadNew(LogTask, NULL, &Log_Task_attributes);

  /* creation of Pressure_Task */
  Pressure_TaskHandle = osThreadNew(PressureTask, NULL, &Pressure_Task_attributes);

  /* creation of Usart2_Task */
  Usart2_TaskHandle = osThreadNew(Usart2Task, NULL, &Usart2_Task_attributes);

  /* creation of LCD_Task */
  LCD_TaskHandle = osThreadNew(LCDTask, NULL, &LCD_Task_attributes);

  /* creation of Monitor_Task */
  Monitor_TaskHandle = osThreadNew(MonitorTask, NULL, &Monitor_Task_attributes);

  /* creation of BLE_Task */
  BLE_TaskHandle = osThreadNew(BLETask, NULL, &BLE_Task_attributes);

  /* creation of DHT11_Task */
  DHT11_TaskHandle = osThreadNew(DHT11Task, NULL, &DHT11_Task_attributes);

  /* creation of FLASH_Task */
  FLASH_TaskHandle = osThreadNew(FLASHTask, NULL, &FLASH_Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  g_PressureTaskHandle = Pressure_TaskHandle;  // 压力任务全局句柄
  g_LCDTaskHandle = LCD_TaskHandle;       // LCD任务全局句柄
  g_DHT11TaskHandle = DHT11_TaskHandle;  // DHT11任务全局句柄
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  
  
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_LogTask */
/**
  * @brief  Function implementing the Log_Task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_LogTask */
void LogTask(void *argument)
{
  /* USER CODE BEGIN LogTask */
  LogMessage_t log_msg;
  char output_buffer[80];
  osStatus_t status;
  
  /* 初始化日志系统 */
  Log_Init();
  
  /* Infinite loop */
  for(;;)
  {
    /* 从队列中获取日志消息 */
    status = osMessageQueueGet(logQueueHandle, &log_msg, NULL, osWaitForever);
    
    if (status == osOK) {
      /* 格式化日志输出 */
      snprintf(output_buffer, sizeof(output_buffer), 
               "[%5lu] %s: %s\r\n", 
               log_msg.timestamp, 
               (log_msg.level < LOG_LEVEL_MAX) ? 
                 ((const char*[]){"ERROR", "WARN ", "INFO ", "DEBUG"})[log_msg.level] : "UNKN",
               log_msg.message);
      
      /* 获取UART1互斥锁 */
      osMutexAcquire(uart1_mutexHandle, osWaitForever);
      
      /* 使用阻塞发送，避免DMA状态检查的死循环 */
      HAL_UART_Transmit(&huart1, (uint8_t*)output_buffer, strlen(output_buffer), 100);
      
      /* 释放UART1互斥锁 */
      osMutexRelease(uart1_mutexHandle);
    }
  }
  /* USER CODE END LogTask */
}

/* USER CODE BEGIN Header_PressureTask */
/**
* @brief Function implementing the Pressure_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_PressureTask */
void PressureTask(void *argument)
{
  /* USER CODE BEGIN PressureTask */
  osDelay(1000);
  Log_Info("Pressure Task Suspend");
  osThreadSuspend(osThreadGetId());
  
  
  double pressure_value;
  uint16_t pressure_value_hpa;
  /* Wait for system initialization */
  osDelay(500);
  
  Log_Info("Pressure Task started");
  
  /* Initialize pressure sensor */
  if (PressureSensor_Init(&hi2c1) != PRESSURE_OK) {
    Log_Error("Pressure sensor initialization failed");
    for(;;) {
      osDelay(1000);
    }
  }
  
  Log_Info("Pressure sensor initialized successfully");
  
  /* Infinite loop */
  for(;;)
  {
      /* 等待任务通知或超时 */
      uint32_t flags = osThreadFlagsWait(0x01, osFlagsWaitAny, 5000);  /* 等待5秒或任务通知 */
      
      if (flags & 0x01) {
          /* 收到任务通知，立即读取压力 */
//          Log_Debug("Pressure task triggered by notification");
      } else {
          /* 超时，正常周期读取 */
//          Log_Debug("Pressure task periodic read");
      }
      
      /* 读取压力数据 */
      pressure_value = PressureSensor_ReadData(&hi2c1);
      
      if (pressure_value != PRESSURE_READ_ERROR)
      {
          /* 更新全局压力值 */
          PressureSensor_UpdateValue(pressure_value);
          
          pressure_value_hpa = pressure_value * 10000; //以hPa为单位
          Log_Debug("Pressure_hpa: %d", pressure_value_hpa);
          Log_Debug("Pressure_mPa: %.4f", pressure_value);
      }
      else
      {
          Log_Error("Pressure sensor read error");
      }
  }
  
  /* USER CODE END PressureTask */
}

/* USER CODE BEGIN Header_Usart2Task */
/**
* @brief Function implementing the Usart2_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Usart2Task */
void Usart2Task(void *argument)
{
  /* USER CODE BEGIN Usart2Task */
  BLEMessage_t ble_msg;
  osStatus_t status;
  uint8_t modbus_response[256];
  uint16_t response_length;
  
  /* 等待系统初始化完成 */
  osDelay(200);
  
  /* 初始化BLE系统 */
  BLE_Init();
  
  /* 初始化Modbus系统 */
  Modbus_Init();
  
  Log_Info("Usart2 Task started, waiting for data from UART2");
  /* Infinite loop */
  for(;;)
  {
    /* 从消息队列获取BLE消息 */
    status = osMessageQueueGet(BLEQueueHandle, &ble_msg, NULL, osWaitForever);
    if (status == osOK)
    {
      /* 先检测是否为Modbus命令 */
      if (Modbus_IsModbusCommand(ble_msg.data, ble_msg.length))
      {
        /* 是Modbus命令，处理Modbus请求 */
        Modbus_ProcessRequest(ble_msg.data, ble_msg.length, modbus_response, &response_length);
        
        if (response_length > 0)
        {
          /* 获取UART1互斥锁 */
          osMutexAcquire(uart1_mutexHandle, osWaitForever);
          
          /* 发送Modbus响应 */
          HAL_UART_Transmit(&huart1, modbus_response, response_length, 100);
          HAL_UART_Transmit(&huart2, modbus_response, response_length, 100);
          /* 释放UART1互斥锁 */
          osMutexRelease(uart1_mutexHandle);
          
          Log_Info("Modbus response sent: %d bytes", response_length);
        }
      }
      else
      {
        /* 不是Modbus命令，作为普通字符串处理 */
        osMutexAcquire(uart1_mutexHandle, osWaitForever);
        
        /* 添加接收标识 */
        HAL_UART_Transmit(&huart1, (uint8_t*)"receive:", 8, 100);
        
        /* 把接收到的数据转发到串口1打印 */
        HAL_UART_Transmit(&huart1, ble_msg.data, ble_msg.length, 100);
        
        /* 添加换行符以便于观察 */
        HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 100);
        
        /* 释放UART1互斥锁 */
        osMutexRelease(uart1_mutexHandle);
        
        Log_Info("String data received: %d bytes", ble_msg.length);
      }
    }
  }
  /* USER CODE END Usart2Task */
}

/* USER CODE BEGIN Header_LCDTask */
/**
* @brief Function implementing the LCD_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LCDTask */
void LCDTask(void *argument)
{
  /* USER CODE BEGIN LCDTask */
  osDelay(1000);
  Log_Info("LCD Task Suspend");
  osThreadSuspend(osThreadGetId());
  
  
  
  double pressure_value;
  float temperature_value;
  float humidity_value;
  char dispBuff[100];
  uint16_t display_counter = 0;
  uint8_t first_run = 1;
  
  ILI9341_Init ();             //LCD 初始化
  ILI9341_GramScan( 6 );
  extern uint16_t lcdid;
  
  Log_Info("LCD Task started");
  
  /* Infinite loop */
  for(;;)
  {
    /* 等待任务通知或超时 */
    uint32_t flags = osThreadFlagsWait(0x01, osFlagsWaitAny, 1000);  /* 等待1秒超时 */
    
    display_counter++;
    
    /* 只在第一次运行时清屏并显示静态内容 */
    if (first_run) {
      /* 清屏 */
      ILI9341_Clear(0,0,LCD_X_LENGTH,LCD_Y_LENGTH);

      first_run = 0;
    }
    
    /* 检查是否收到任务通知 */
    if (flags & 0x01) {
//      Log_Debug("LCD task triggered by notification");
    } else {
//      Log_Debug("LCD task periodic refresh");
    }
    
    /* 直接从全局变量读取压力数据 */
    pressure_value = PressureSensor_GetLatestValue();
    
    if (pressure_value != PRESSURE_READ_ERROR) {
      /* 使用OpenWindow精确刷新压力数据区域 */
      ILI9341_OpenWindow(0, LINE(0), LCD_X_LENGTH, LINE(2) - LINE(0) + 16);
      ILI9341_Clear(0, LINE(0), LCD_X_LENGTH, LINE(2) - LINE(0) + 16);  /* 清除压力数据区域 */
      
      /* 显示压力数据 */
      LCD_SetFont(&Font8x16);
      LCD_SetTextColor(GREEN);
      ILI9341_DispStringLine_EN(LINE(0),"Pressure Data:");
      
      LCD_SetFont(&Font8x16);
      LCD_SetTextColor(YELLOW);
      sprintf(dispBuff,"%.4f MPa", pressure_value);
      ILI9341_DispStringLine_EN(LINE(1), dispBuff);
 
    } 

    /* 直接从全局变量读取温度数据 */
    temperature_value = TemperatureSensor_GetLatestValue();
    if (temperature_value != 0.0f) {
      /* 使用OpenWindow精确刷新温度数据区域 */
      ILI9341_OpenWindow(0, LINE(2), LCD_X_LENGTH, LINE(2) - LINE(2) + 16);
      ILI9341_Clear(0, LINE(2), LCD_X_LENGTH, LINE(2) - LINE(2) + 16);  /* 清除温度数据区域 */
      
      /* 显示温度数据 */
      LCD_SetFont(&Font8x16);
      LCD_SetTextColor(GREEN);
      ILI9341_DispStringLine_EN(LINE(2),"Temperature Data:");
      
      LCD_SetFont(&Font8x16);
      LCD_SetTextColor(YELLOW);
      sprintf(dispBuff,"%.1f C", temperature_value);
      ILI9341_DispStringLine_EN(LINE(3), dispBuff);
    }

    
    /* 直接从全局变量读取湿度数据 */
    humidity_value = HumiditySensor_GetLatestValue();
    if (humidity_value != 0.0f) {
      /* 使用OpenWindow精确刷新湿度数据区域 */
      ILI9341_OpenWindow(0, LINE(4), LCD_X_LENGTH, LINE(4) - LINE(4) + 16);
      ILI9341_Clear(0, LINE(4), LCD_X_LENGTH, LINE(4) - LINE(4) + 16);  /* 清除湿度数据区域 */
      
      /* 显示湿度数据 */
      LCD_SetFont(&Font8x16);
      LCD_SetTextColor(GREEN);
      ILI9341_DispStringLine_EN(LINE(4),"Humidity Data:");
      
      LCD_SetFont(&Font8x16);
      LCD_SetTextColor(YELLOW);
      sprintf(dispBuff,"%.1f%%", humidity_value);
      ILI9341_DispStringLine_EN(LINE(5), dispBuff);
    }


    /* 使用OpenWindow精确刷新系统信息区域 */
    ILI9341_OpenWindow(0, LINE(19), LCD_X_LENGTH, LINE(19) - LINE(19) + 16);
    ILI9341_Clear(0, LINE(19), LCD_X_LENGTH, LINE(19) - LINE(19) + 16);  /* 清除系统信息区域 */
    
    /* 显示计数器 */
    LCD_SetFont(&Font8x16);
    LCD_SetTextColor(WHITE);
    sprintf(dispBuff,"Display Count: %d", display_counter);
    ILI9341_DispStringLine_EN(LINE(19), dispBuff);
    
    /* 注意：不再需要osDelay，因为osThreadFlagsWait已经处理了超时 */
  }
  /* USER CODE END LCDTask */
}

/* USER CODE BEGIN Header_MonitorTask */
/**
* @brief Function implementing the Monitor_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_MonitorTask */
void MonitorTask(void *argument)
{
  /* USER CODE BEGIN MonitorTask */
  osDelay(1000);
  Log_Info("Monitor Task Suspend");
  osThreadSuspend(osThreadGetId());
  
  
  /* Infinite loop */
  for(;;)
  {
    osDelay(1000);
  }
  /* USER CODE END MonitorTask */
}

/* USER CODE BEGIN Header_BLETask */
/**
* @brief Function implementing the BLE_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_BLETask */
void BLETask(void *argument)
{
  /* USER CODE BEGIN BLETask */
  osDelay(2000);
  Log_Info("BLE Task Suspend");
  osThreadSuspend(osThreadGetId());
  
  
  
  Log_Info("BLETask Start");

  HAL_UART_Transmit(&huart2, (uint8_t*)"+++", 3, 100);
  osDelay(1000);
  HAL_UART_Transmit(&huart2, (uint8_t*)"AT+NAME=0,STM32F103VETx\r\n", 25, 100);
  osDelay(1000);
  HAL_UART_Transmit(&huart2, (uint8_t*)"AT+ROLE=0\r\n", 11, 100);
  osDelay(1000);
  HAL_UART_Transmit(&huart2, (uint8_t*)"AT+EXIT\r\n", 9, 100);

  
  /* Infinite loop */
  for(;;)
  {

    osDelay(1000);
  }
  /* USER CODE END BLETask */
}

/* USER CODE BEGIN Header_DHT11Task */
/**
* @brief Function implementing the DHT11_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_DHT11Task */
void DHT11Task(void *argument)
{
  /* USER CODE BEGIN DHT11Task */
  
  osDelay(1000);
  Log_Info("DHT11 Task Suspend");
  osThreadSuspend(osThreadGetId());
  
  
  float temperature = 0.0f;
  float humidity = 0.0f;
  uint8_t read_result = 0;
  uint32_t read_count = 0;
  uint16_t counter = 0;
  
  osDelay(2000);
  Log_Info("DHT11Task Start");
  
  /* 初始化DWT延时 */
  DWT_Init();
  
  /* 初始化DHT11 */
  DHT11_Init();
  
  /* Infinite loop */
  for(;;)
  {
    counter ++;
    /* 等待任务通知或超时 */
    uint32_t flags = osThreadFlagsWait(0x01, osFlagsWaitAny, 5000);  /* 等待5秒或任务通知 */
    
    Log_Debug("counter : %d",counter);
    read_count++;
    
    /* 检查是否收到了任务通知 */
    if (flags & 0x01) {
      Log_Info("DHT11 triggered by task notification (attempt #%lu)", read_count);
    } else {
      Log_Info("DHT11 triggered by timeout (attempt #%lu)", read_count);
    }
    
    /* 读取DHT11数据 */
    read_result = DHT11_Read_Data(&temperature, &humidity);
    
    if (read_result == 0)
    {
      SensorData_UpdateTemperature(temperature);
      SensorData_UpdateHumidity(humidity);
    }
    else
    {
      Log_Error("DHT11 read failed (attempt #%lu)", read_count);
    }
  }
  /* USER CODE END DHT11Task */
}

/* USER CODE BEGIN Header_FLASHTask */
/**
* @brief Function implementing the FLASH_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_FLASHTask */
void FLASHTask(void *argument)
{
  /* USER CODE BEGIN FLASHTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END FLASHTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

