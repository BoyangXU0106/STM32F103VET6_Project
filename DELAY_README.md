# 延时函数库 (Delay Library)

## 概述

本延时函数库基于STM32F103VET6的TIM3定时器实现，提供精确的微秒级和毫秒级延时功能。特别适用于单总线协议（如DS18B20温度传感器）等需要精确时序控制的场合。

## 文件结构

```
mycodeh/
├── delay.h          # 延时函数头文件
mycodec/
├── delay.c          # 延时函数实现
└── delay_test.c     # 延时函数测试代码
```

## 功能特性

### 1. 微秒级延时 (Delay_Us)
- **精度**: 1微秒
- **范围**: 1-65535微秒
- **实现**: 基于TIM3定时器
- **适用**: 单总线协议、精确时序控制

### 2. 毫秒级延时 (Delay_Ms)
- **精度**: 1毫秒
- **范围**: 1-65535毫秒
- **实现**: 基于HAL_Delay
- **适用**: 一般延时需求

## 硬件配置

### TIM3配置
- **时钟源**: APB1 (36MHz)
- **预分频器**: 35
- **实际时钟**: 1MHz (36MHz / 36)
- **计数器周期**: 65535
- **精度**: 1微秒/计数

### 系统时钟配置
- **SYSCLK**: 72MHz
- **APB1**: 36MHz
- **APB2**: 72MHz

## 使用方法

### 1. 包含头文件
```c
#include "delay.h"
```

### 2. 微秒延时示例
```c
// 延时1微秒
Delay_Us(1);

// 延时10微秒
Delay_Us(10);

// 延时100微秒
Delay_Us(100);

// 延时1000微秒 (1毫秒)
Delay_Us(1000);
```

### 3. 毫秒延时示例
```c
// 延时1毫秒
Delay_Ms(1);

// 延时10毫秒
Delay_Ms(10);

// 延时100毫秒
Delay_Ms(100);
```

### 4. 单总线协议应用示例
```c
// DS18B20温度传感器时序控制
void DS18B20_WriteBit(uint8_t bit)
{
    // 拉低总线
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    Delay_Us(1);  // 精确1微秒延时
    
    if (bit) {
        // 写'1'
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        Delay_Us(59);  // 精确59微秒延时
    } else {
        // 写'0'
        Delay_Us(60);  // 精确60微秒延时
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    }
    
    Delay_Us(1);  // 恢复时间
}
```

## 实现原理

### 微秒延时实现
1. **启动TIM3**: 使用`HAL_TIM_Base_Start()`
2. **获取起始时间**: 读取TIM3计数器值
3. **循环等待**: 持续读取计数器值直到达到目标延时
4. **处理溢出**: 自动处理16位计数器溢出情况
5. **停止TIM3**: 使用`HAL_TIM_Base_Stop()`

### 溢出处理
```c
// 处理计数器溢出情况
if (current_time < start_time) {
    // 计数器溢出，计算剩余延时时间
    uint16_t elapsed = (65535 - start_time) + current_time + 1;
    if (elapsed >= us) {
        break;
    }
    us -= elapsed;
    start_time = current_time;
}
```

## 性能特点

### 优点
- ✅ **高精度**: 1微秒精度
- ✅ **低开销**: 基于硬件定时器
- ✅ **自动溢出处理**: 支持长时间延时
- ✅ **参数检查**: 防止无效参数
- ✅ **资源管理**: 自动启动/停止定时器

### 注意事项
- ⚠️ **占用TIM3**: 延时期间TIM3被占用
- ⚠️ **中断影响**: 高优先级中断可能影响精度
- ⚠️ **范围限制**: 单次最大延时65535微秒

## 测试验证

### 测试函数
```c
// 基本功能测试
Delay_Test();

// 性能测试
Delay_PerformanceTest();
```

### 测试内容
1. **基本延时测试**: 1us, 10us, 100us, 1000us
2. **毫秒延时测试**: 1ms, 10ms
3. **性能验证**: 实际延时时间测量

## 集成说明

### 1. 添加到项目
1. 将`delay.h`添加到`mycodeh/`目录
2. 将`delay.c`添加到`mycodec/`目录
3. 在需要使用的文件中包含`#include "delay.h"`

### 2. 确保TIM3已初始化
```c
// 在main.c中确保TIM3已初始化
MX_TIM3_Init();
```

### 3. 编译配置
- 确保项目包含`tim.h`和`tim.c`
- 确保TIM3相关配置正确

## 应用场景

1. **单总线协议**: DS18B20温度传感器
2. **精确时序控制**: 步进电机控制
3. **通信协议**: I2C、SPI时序控制
4. **传感器接口**: 各种传感器时序要求
5. **测试测量**: 精确时间测量

## 版本信息

- **版本**: 1.0.0
- **作者**: STM32开发团队
- **日期**: 2025年
- **兼容性**: STM32F103VET6, HAL库

## 许可证

Copyright (c) 2025 STMicroelectronics. All rights reserved.
