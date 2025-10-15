/* pressure_task.h */
#ifndef __PRESSURE_TASK_H
#define __PRESSURE_TASK_H

#include "main.h"
#include "cmsis_os.h"
#include "pressure.h"

// 压力数据结构
typedef struct {
    float pressure_value;    // 压力值 (MPa)
    uint8_t valid;          // 数据有效性标志
    uint32_t timestamp;     // 时间戳
} pressure_data_t;

// 压力任务相关函数声明
void pressure_task_init(void);
void pressure_task(void *argument);

// 压力队列句柄声明
extern osMessageQueueId_t pressureQueueHandle;

#endif /* __PRESSURE_TASK_H */
