/**
  * @file    bsp_dht11.c
  * @author  Your Name
  * @version V1.0
  * @date    2024-01-01
  * @brief   DHT11温湿度传感器驱动
  * @note    使用DWT延时实现精确时序控制
  */

#include "bsp_dht11.h"
#include "bsp_dwt.h"
#include "log.h"
#include "modbus.h"

/* 私有函数声明 */
static void DHT11_GPIO_Init(void);
static void DHT11_Set_Output(void);
static void DHT11_Set_Input(void);
static uint8_t DHT11_Read_Byte(void);
static uint8_t DHT11_Check_Response(void);

/**
  * @brief  DHT11初始化
  * @param  无
  * @retval 无
  * @note   初始化DHT11相关的GPIO
  */
void DHT11_Init(void)
{
    DHT11_GPIO_Init();
    Log_Info("DHT11 initialized successfully");
}

/**
  * @brief  DHT11读取温湿度数据
  * @param  temp: 温度数据指针
  * @param  humi: 湿度数据指针
  * @retval 0: 成功, 1: 失败
  * @note   读取DHT11的温湿度数据
  */
uint8_t DHT11_Read_Data(float *temp, float *humi)
{
    uint8_t data[5] = {0};
    uint8_t i;
    
    /* 发送开始信号 */
    DHT11_Set_Output();
    DHT11_GPIO_RESET();        // 拉低数据线
    DWT_DelayMs(18);           // 延时18ms
    DHT11_GPIO_SET();          // 拉高数据线
    DWT_DelayUs(30);           // 延时30us
    
    /* 等待DHT11响应 */
    DHT11_Set_Input();
    if (DHT11_Check_Response() == 0)
    {
        Log_Error("DHT11 response timeout");
        return 1;
    }
    
    /* 读取40位数据 */
    for (i = 0; i < 5; i++)
    {
        data[i] = DHT11_Read_Byte();
    }
    
    /* 校验数据 */
    if (data[4] != (data[0] + data[1] + data[2] + data[3]))
    {
        Log_Error("DHT11 checksum error");
        return 1;
    }
    
    /* 计算温湿度值 */
    *humi = (float)data[0] + (float)data[1] / 10.0;
    *temp = (float)data[2] + (float)data[3] / 10.0;
    
    Log_Info("DHT11 read success: temp=%.1f C, humi=%.1f%%", *temp, *humi);
    return 0;
}

/**
  * @brief  DHT11 GPIO初始化
  * @param  无
  * @retval 无
  * @note   初始化DHT11数据引脚
  */
static void DHT11_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* 使能GPIO时钟 */
    DHT11_GPIO_CLK_ENABLE();
    
    /* 配置GPIO */
    GPIO_InitStruct.Pin = DHT11_GPIO_PIN_NUM;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
    
    /* 初始状态拉高 */
    DHT11_GPIO_SET();
}

/**
  * @brief  设置DHT11为输出模式
  * @param  无
  * @retval 无
  * @note   配置GPIO为输出模式
  */
static void DHT11_Set_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Pin = DHT11_GPIO_PIN_NUM;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

/**
  * @brief  设置DHT11为输入模式
  * @param  无
  * @retval 无
  * @note   配置GPIO为输入模式
  */
static void DHT11_Set_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Pin = DHT11_GPIO_PIN_NUM;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

/**
  * @brief  检查DHT11响应
  * @param  无
  * @retval 0: 超时, 1: 正常响应
  * @note   检查DHT11是否正常响应
  */
static uint8_t DHT11_Check_Response(void)
{
    uint32_t timeout = 0;
    
    /* 等待DHT11拉低数据线 */
    while (DHT11_GPIO_READ() == GPIO_PIN_SET && timeout < 100)
    {
        DWT_DelayUs(1);
        timeout++;
    }
    if (timeout >= 100) return 0;
    
    timeout = 0;
    /* 等待DHT11拉高数据线 */
    while (DHT11_GPIO_READ() == GPIO_PIN_RESET && timeout < 100)
    {
        DWT_DelayUs(1);
        timeout++;
    }
    if (timeout >= 100) return 0;
    
    return 1;
}

/**
  * @brief  读取一个字节数据
  * @param  无
  * @retval 读取的字节数据
  * @note   读取DHT11发送的一个字节数据
  */
static uint8_t DHT11_Read_Byte(void)
{
    uint8_t data = 0;
    uint8_t i;
    
    for (i = 0; i < 8; i++)
    {
        /* 等待数据线拉低 */
        while (DHT11_GPIO_READ() == GPIO_PIN_SET);
        
        /* 等待数据线拉高 */
        while (DHT11_GPIO_READ() == GPIO_PIN_RESET);
        
        /* 延时30us后读取数据 */
        DWT_DelayUs(30);
        
        /* 判断数据位 */
        if (DHT11_GPIO_READ() == GPIO_PIN_SET)
        {
            data |= (1 << (7 - i));
        }
        
        /* 等待数据位结束 */
        while (DHT11_GPIO_READ() == GPIO_PIN_SET);
    }
    
    return data;
}


/**
 * @brief 获取最新的温度值
 * @return 温度值（°C）
 */
float TemperatureSensor_GetLatestValue(void)
{
    return g_sensor_data.temperature;
}

/**
 * @brief 获取最新的湿度值
 * @return 湿度值（%）
 */
float HumiditySensor_GetLatestValue(void)
{
    return g_sensor_data.humidity;
}

