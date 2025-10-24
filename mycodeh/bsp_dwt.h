/**
  * @file    bsp_dwt.h
  * @author  Your Name
  * @version V1.0
  * @date    2024-01-01
  * @brief   DWT延时函数头文件
  * @note    使用DWT计数器实现精确的微秒级延时
  */

#ifndef __BSP_DWT_H
#define __BSP_DWT_H

#include "stm32f1xx_hal.h"

/* 函数声明 */
void DWT_Init(void);
uint32_t DWT_GetTick(void);
void DWT_DelayUs(uint32_t us);
void DWT_DelayMs(uint32_t ms);

#endif /* __BSP_DWT_H */
