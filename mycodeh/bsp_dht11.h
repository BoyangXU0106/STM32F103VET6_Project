/**
  * @file    bsp_dht11.h
  * @author  Your Name
  * @version V1.0
  * @date    2024-01-01
  * @brief   DHT11温湿度传感器驱动头文件
  * @note    使用DWT延时实现精确时序控制
  */

#ifndef __BSP_DHT11_H
#define __BSP_DHT11_H

#include "stm32f1xx_hal.h"

/* DHT11 GPIO定义 */
#define DHT11_GPIO_PORT        GPIOE
#define DHT11_GPIO_PIN_NUM     GPIO_PIN_6
#define DHT11_GPIO_CLK_ENABLE() __HAL_RCC_GPIOE_CLK_ENABLE()

/* GPIO操作宏定义 */
#define DHT11_GPIO_SET()       HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN_NUM, GPIO_PIN_SET)
#define DHT11_GPIO_RESET()     HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN_NUM, GPIO_PIN_RESET)
#define DHT11_GPIO_READ()      HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN_NUM)

/* 函数声明 */
void DHT11_Init(void);
uint8_t DHT11_Read_Data(float *temp, float *humi);
float TemperatureSensor_GetLatestValue(void);
float HumiditySensor_GetLatestValue(void);

#endif /* __BSP_DHT11_H */
