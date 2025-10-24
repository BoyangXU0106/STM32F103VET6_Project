/**
  * @file    bsp_dwt.c
  * @author  Your Name
  * @version V1.0
  * @date    2024-01-01
  * @brief   DWT延时函数实现
  * @note    使用DWT计数器实现精确的微秒级延时
  */

#include "bsp_dwt.h"
#include "log.h"

/**
  * @brief  DWT初始化
  * @param  无
  * @retval 无
  * @note   使能DWT计数器
  */
void DWT_Init(void)
{
    /* 使能DWT计数器 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    
    /* 清零DWT计数器 */
    DWT->CYCCNT = 0;
    
    /* 使能DWT计数器 */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    
    Log_Info("DWT initialized successfully");
}

/**
  * @brief  获取DWT计数器值
  * @param  无
  * @retval DWT计数器值
  * @note   返回当前DWT计数器的值
  */
uint32_t DWT_GetTick(void)
{
    return DWT->CYCCNT;
}

/**
  * @brief  DWT微秒延时
  * @param  us: 延时时间，单位微秒
  * @retval 无
  * @note   使用DWT计数器实现精确的微秒级延时
  */
void DWT_DelayUs(uint32_t us)
{
    uint32_t start_tick = DWT_GetTick();
    uint32_t delay_ticks = us * (SystemCoreClock / 1000000);
    
    /* 等待延时完成 */
    while ((DWT_GetTick() - start_tick) < delay_ticks)
    {
        /* 空循环等待 */
    }
}

/**
  * @brief  DWT毫秒延时
  * @param  ms: 延时时间，单位毫秒
  * @retval 无
  * @note   使用DWT计数器实现精确的毫秒级延时
  */
void DWT_DelayMs(uint32_t ms)
{
    DWT_DelayUs(ms * 1000);
}