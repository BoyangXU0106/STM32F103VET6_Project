/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : ble_data.c
  * @brief          : BLE data management implementation
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
#include "ble_data.h"
#include "usart.h"
#include "dma.h"
#include "log.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cmsis_os.h"
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

/* 外部变量声明 */
extern uint8_t bt_rx_buffer[256];
extern osMessageQueueId_t BLEQueueHandle;

/* BLE全局变量定义 */
uint8_t ble_rx_buffer[BLE_RX_BUFFER_SIZE];
uint8_t ble_tx_buffer[BLE_TX_BUFFER_SIZE];
volatile uint16_t ble_rx_count = 0;
volatile uint8_t ble_rx_complete = 0;
volatile BLE_RxState_t ble_rx_state = BLE_RX_IDLE;

/* 串口1独立缓冲区 */
uint8_t uart1_rx_buffer[1];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  BLE系统初始化
  * @retval None
  */
void BLE_Init(void)
{
    /* 清零缓冲区 */
    memset(ble_rx_buffer, 0, BLE_RX_BUFFER_SIZE);
    memset(ble_tx_buffer, 0, BLE_TX_BUFFER_SIZE);
    
    /* 初始化状态 */
    ble_rx_count = 0;
    ble_rx_complete = 0;
    ble_rx_state = BLE_RX_IDLE;
    
    Log_Info("BLE system initialized");
    
    /* 启动UART2 DMA接收 - 使用ReceiveToIdle模式 */
    if (HAL_UARTEx_ReceiveToIdle_DMA(&huart2, bt_rx_buffer, sizeof(bt_rx_buffer)) != HAL_OK) {
        Log_Error("UART2 DMA receive start failed");
    } else {
        Log_Debug("UART2 DMA receive started successfully");
        /* 禁止半传输中断 */
        __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
    }
}

/**
  * @brief  BLE UART接收完成回调函数
  * @param  huart: UART句柄指针
  * @retval None
  */
void BLE_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        /* 获取DMA传输的字节数 */
        ble_rx_count = BLE_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart2.hdmarx);
        
        Log_Debug("BLE UART Rx Complete: %d bytes", ble_rx_count);
        
        /* 创建BLE消息并复制实际接收的数据 */
        BLEMessage_t ble_msg;
        ble_msg.length = ble_rx_count;
        ble_msg.timestamp = HAL_GetTick();
        
        /* 复制接收到的数据 */
        memcpy(ble_msg.data, ble_rx_buffer, ble_rx_count);
        
        /* 将消息放入队列 */
        osStatus_t status = osMessageQueuePut(BLEQueueHandle, &ble_msg, 0, 0);
        if (status != osOK) {
            Log_Error("BLE queue full, data lost");
        } else {
            Log_Debug("BLE data queued: %d bytes", ble_rx_count);
        }
        
        /* 设置接收完成标志 */
        ble_rx_complete = 1;
        
        /* 重新启动接收 */
        UART2_StartReceive();
    }
}

/**
  * @brief  UART接收事件回调函数 (用于ReceiveToIdle模式)
  * @param  huart: UART句柄指针
  * @param  Size: 接收到的数据大小
  * @retval None
  */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1) // 调试串口
    {
        Log_Debug("UART1 Rx Event: %d bytes", Size);
        // 这里可以添加USART1的处理逻辑
    }
    else if (huart->Instance == USART2) // 蓝牙串口
    {
        Log_Debug("BLE UART Rx Event: %d bytes", Size);
        
        // 创建BLE消息并复制实际接收的数据
        BLEMessage_t ble_msg;
        ble_msg.length = Size;
        ble_msg.timestamp = HAL_GetTick();
        
        // 只复制实际接收到的数据
        memcpy(ble_msg.data, bt_rx_buffer, Size);
        
        // 将消息放入队列
        osStatus_t status = osMessageQueuePut(BLEQueueHandle, &ble_msg, 0, 0);
        if (status != osOK) {
            Log_Error("BLE queue full, data lost");
        } else {
            Log_Debug("BLE data queued: %d bytes", Size);
        }
        
        // 重新启动DMA接收
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, bt_rx_buffer, sizeof(bt_rx_buffer));
        __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT); // 禁止半传输中断
    }
}

/**
  * @brief  启动串口2的DMA接收
  * @retval None
  */
void UART2_StartReceive(void)
{
    /* 清零接收计数器和完成标志 */
    ble_rx_count = 0;
    ble_rx_complete = 0;
    ble_rx_state = BLE_RX_RECEIVING;
    
    Log_Debug("Starting UART2 DMA receive, buffer size: %d", BLE_RX_BUFFER_SIZE);
    
    /* 启动DMA接收 */
    if (HAL_UART_Receive_DMA(&huart2, ble_rx_buffer, BLE_RX_BUFFER_SIZE) != HAL_OK) {
        Log_Error("UART2 DMA receive start failed");
        ble_rx_state = BLE_RX_ERROR;
    } else {
        Log_Debug("UART2 DMA receive started successfully");
    }
}

/**
  * @brief  停止串口2的DMA接收
  * @retval None
  */
void UART2_StopReceive(void)
{
    /* 停止DMA接收 */
    HAL_UART_DMAStop(&huart2);
    ble_rx_state = BLE_RX_IDLE;
    Log_Debug("UART2 DMA receive stopped");
}

/**
  * @brief  处理接收到的数据
  * @retval None
  */
void UART2_ProcessReceivedData(void)
{
    if (ble_rx_complete && ble_rx_count > 0) {
        Log_Debug("UART2_ProcessReceivedData: Processing %d bytes", ble_rx_count);
        
        /* 创建BLE消息 */
        BLEMessage_t ble_msg;
        ble_msg.length = ble_rx_count;
        ble_msg.timestamp = HAL_GetTick();
        
        /* 复制接收到的数据 */
        memcpy(ble_msg.data, ble_rx_buffer, ble_rx_count);
        
        /* 发送到BLE队列 */
        osStatus_t status = osMessageQueuePut(BLEQueueHandle, &ble_msg, 0, 0);
        if (status != osOK) {
            Log_Error("BLE queue full, data lost");
        } else {
            Log_Debug("BLE data queued: %d bytes", ble_rx_count);
        }
        
        /* 清除完成标志 */
        ble_rx_complete = 0;
        ble_rx_count = 0;
        
        /* 重新启动接收 */
        UART2_StartReceive();
    }
}

/**
  * @brief  串口2发送数据
  * @param  data: 要发送的数据
  * @param  length: 数据长度
  * @retval HAL状态
  */
HAL_StatusTypeDef UART2_SendData(uint8_t *data, uint16_t length)
{
    if (data == NULL || length == 0) {
        return HAL_ERROR;
    }
    
    /* 使用普通HAL_UART_Transmit发送 */
    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart2, data, length, 1000);
    
    if (status != HAL_OK) {
        Log_Error("UART2 send failed");
    }
    
    return status;
}

/**
  * @brief  串口2发送字符串
  * @param  str: 要发送的字符串
  * @retval HAL状态
  */
HAL_StatusTypeDef UART2_SendString(const char *str)
{
    if (str == NULL) {
        return HAL_ERROR;
    }
    
    return UART2_SendData((uint8_t*)str, strlen(str));
}

/**
  * @brief  处理BLE消息
  * @param  msg: BLE消息指针
  * @retval None
  */
void BLE_ProcessMessage(BLEMessage_t *msg)
{
    Log_Debug("BLE_ProcessMessage called: %d bytes", msg->length);
    
    if (msg == NULL || msg->length == 0) {
        return;
    }
    
    /* 确保字符串以null结尾 */
    uint8_t temp_data[BLE_RX_BUFFER_SIZE + 1];
    memcpy(temp_data, msg->data, msg->length);
    temp_data[msg->length] = '\0';
    
    /* 转发到日志任务 */
    Log_Info("BLE RX: %.*s", msg->length, temp_data);
    
    /* 转发到串口1（调试输出） */
    char debug_msg[64];
    snprintf(debug_msg, sizeof(debug_msg), "BLE[%lu]: %.*s\r\n", 
             msg->timestamp, msg->length, temp_data);
    
    /* 使用阻塞发送，增加超时时间 */
    HAL_StatusTypeDef uart_status = HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
    if (uart_status != HAL_OK) {
        Log_Error("UART1 transmit failed: %d", uart_status);
    }
    
    /* 回显到串口2 */
    char echo_msg[32];
    snprintf(echo_msg, sizeof(echo_msg), "Echo: %.*s\r\n", 
             msg->length, temp_data);
    UART2_SendString(echo_msg);
}

/* USER CODE END 0 */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */


/**
  * @brief  UART2错误回调函数
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        Log_Error("UART2 error occurred");
        ble_rx_state = BLE_RX_ERROR;
        
        /* 重新启动接收 */
        UART2_StartReceive();
    }
}

/* USER CODE END Application */
