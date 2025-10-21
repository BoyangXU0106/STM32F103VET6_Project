/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : ble_data.h
  * @brief          : BLE data structures and definitions
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

#ifndef __BLE_DATA_H
#define __BLE_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* BLE消息结构体 */
typedef struct {
    uint8_t data[256];      /* 数据缓冲区 */
    uint16_t length;        /* 数据长度 */
    uint32_t timestamp;     /* 时间戳 */
} BLEMessage_t;

/* BLE接收状态 */
typedef enum {
    BLE_RX_IDLE = 0,
    BLE_RX_RECEIVING,
    BLE_RX_COMPLETE,
    BLE_RX_ERROR
} BLE_RxState_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* BLE配置参数 */
#define BLE_RX_BUFFER_SIZE     256
#define BLE_TX_BUFFER_SIZE     256
#define BLE_QUEUE_SIZE         5
#define BLE_MESSAGE_SIZE       256

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* BLE全局变量 */
extern uint8_t ble_rx_buffer[BLE_RX_BUFFER_SIZE];
extern uint8_t ble_tx_buffer[BLE_TX_BUFFER_SIZE];
extern volatile uint16_t ble_rx_count;
extern volatile uint8_t ble_rx_complete;
extern volatile BLE_RxState_t ble_rx_state;

/* BLE队列句柄 */
extern osMessageQueueId_t BLEQueueHandle;

/* USER CODE END EV */

/* Exported function prototypes ----------------------------------------------*/
/* USER CODE BEGIN EFP */

/* BLE初始化函数 */
void BLE_Init(void);

/* 串口2接收函数 */
void UART2_StartReceive(void);
void UART2_StopReceive(void);
void UART2_ProcessReceivedData(void);

/* 串口2发送函数 */
HAL_StatusTypeDef UART2_SendData(uint8_t *data, uint16_t length);
HAL_StatusTypeDef UART2_SendString(const char *str);

/* BLE数据处理函数 */
void BLE_ProcessMessage(BLEMessage_t *msg);

/* BLE UART回调函数 */
void BLE_UART_RxCpltCallback(UART_HandleTypeDef *huart);

/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /* __BLE_DATA_H */
