/* pressure_task.c */
#include "pressure_task.h"
#include "log.h"
#include "main.h"

// 外部I2C句柄声明
extern I2C_HandleTypeDef hi2c2;

// 压力队列句柄
osMessageQueueId_t pressureQueueHandle;

// 压力队列属性
const osMessageQueueAttr_t pressureQueue_attributes = {
    .name = "pressureQueue"
};


/**
 * @brief 压力任务初始化
 */
void pressure_task_init(void)
{
    // 创建压力数据队列
    pressureQueueHandle = osMessageQueueNew(5, sizeof(pressure_data_t), &pressureQueue_attributes);
    if (pressureQueueHandle == NULL) {
        LOG_ERROR("Failed to create pressure queue!");
        return;
    }
    
    LOG_INFO("Pressure task initialized successfully");
}

/**
 * @brief 压力采集任务
 * @param argument: 任务参数
 */
void pressure_task(void *argument)
{
    (void)argument;
    
    pressure_data_t pressure_data;
    uint32_t last_collect_time = 0;
    const uint32_t COLLECT_INTERVAL = 5000; // 5秒采集一次
    
    LOG_INFO("Pressure task started - Task ID: %p", osThreadGetId());
    
    // 添加栈使用监控
    UBaseType_t stack_high_water_mark;
    stack_high_water_mark = uxTaskGetStackHighWaterMark(NULL);
    LOG_DEBUG("Pressure task stack high water mark: %lu", stack_high_water_mark);
    
    // 等待系统稳定
    osDelay(2000);
    
    LOG_INFO("Pressure task ready to initialize sensor");
    
    // 初始化压力传感器
    if (PressureSensor_Init(&hi2c2) != PRESSURE_OK) {
        LOG_ERROR("Pressure sensor initialization failed!");
        
        // 发送错误数据到队列
        pressure_data.pressure_value = 0.0f;
        pressure_data.valid = 0;
        pressure_data.timestamp = osKernelGetTickCount();
        osMessageQueuePut(pressureQueueHandle, &pressure_data, 0, 0);
        
        // 任务进入错误状态，每10秒重试一次
        for(;;) {
            osDelay(10000);
            if (PressureSensor_Init(&hi2c2) == PRESSURE_OK) {
                LOG_INFO("Pressure sensor re-initialized successfully");
                break;
            }
        }
    } else {
        LOG_INFO("Pressure sensor initialized successfully");
    }
    
    for(;;) {
        uint32_t current_time = osKernelGetTickCount();
        
         // 检查是否到了采集时间
         if (current_time - last_collect_time >= COLLECT_INTERVAL) {
             // 采集压力数据
             double pressure_raw;
             Pressure_Status status = PressureSensor_ReadData(&hi2c2, &pressure_raw);
             
             if (status == PRESSURE_OK) {
                 // 数据采集成功 - 直接使用原始值
                 pressure_data.pressure_value = (float)pressure_raw;
                 pressure_data.valid = 1;
                 pressure_data.timestamp = current_time;
                 
                 LOG_DEBUG("Pressure raw value collected: %.0f", pressure_data.pressure_value);
             } else {
                 // 数据采集失败
                 pressure_data.pressure_value = 0.0f;
                 pressure_data.valid = 0;
                 pressure_data.timestamp = current_time;
                 
                 LOG_ERROR("Pressure collection failed");
             }
            
            // 发送数据到队列
            osStatus_t queue_status = osMessageQueuePut(pressureQueueHandle, &pressure_data, 0, 100);
            if (queue_status != osOK) {
                LOG_ERROR("Failed to put pressure data to queue");
            }
            
            last_collect_time = current_time;
        }
        
        // 任务延迟
        osDelay(100); // 100ms检查一次
    }
}
