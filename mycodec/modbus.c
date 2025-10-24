#include "modbus.h"
#include "log.h"
#include "pressure.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "bsp_dht11.h"
/* External task handles for task notification */
extern osThreadId_t g_PressureTaskHandle;
extern osThreadId_t g_LCDTaskHandle;
extern osThreadId_t g_DHT11TaskHandle;

// 全局传感器数据
GlobalSensorData_t g_sensor_data = {0};        // 全局传感器数据
ModbusRegisters_t g_modbus_registers = {0};    // Modbus寄存器数据

// CRC16查找表
static const uint16_t crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

/**
 * @brief 初始化Modbus模块
 */
void Modbus_Init(void)
{
    // 初始化全局传感器数据
    SensorData_Init();
    
    // 初始化Modbus寄存器数据
    g_modbus_registers.pressure = 0;
    g_modbus_registers.temperature = 0;
    g_modbus_registers.status = 0;
    g_modbus_registers.error_count = 0;
    
    Log_Info("Modbus and global sensor data initialized");
}

/**
 * @brief 更新压力数据
 * @param pressure_value 压力值（浮点数）
 */
void Modbus_UpdatePressure(float pressure_value)
{
    // 使用全局传感器数据更新函数
    SensorData_UpdatePressure((double)pressure_value);
}

/**
 * @brief 更新温度数据
 * @param temperature_value 温度值（浮点数）
 */
void Modbus_UpdateTemperature(float temperature_value)
{
    // 使用全局传感器数据更新函数
    SensorData_UpdateTemperature((double)temperature_value);
}

/**
 * @brief 更新状态数据
 * @param status_value 状态值
 */
void Modbus_UpdateStatus(uint16_t status_value)
{
    // 使用全局传感器数据更新函数
    SensorData_UpdateSystemStatus(status_value);
}

/**
 * @brief 计算CRC16校验
 * @param data 数据指针
 * @param length 数据长度
 * @return CRC16校验值
 */
uint16_t Modbus_CalculateCRC16(uint8_t* data, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    
    for (uint16_t i = 0; i < length; i++)
    {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc16_table[index];
    }
    
    return crc;
}

/**
 * @brief 验证CRC校验
 * @param frame 帧数据
 * @param length 帧长度
 * @return true: 校验正确, false: 校验错误
 */
bool Modbus_ValidateCRC(uint8_t* frame, uint16_t length)
{
    if (length < 3) return false;
    
    // 尝试两种CRC字节序：高字节在前和低字节在前
    uint16_t received_crc_high_first = (frame[length - 2] << 8) | frame[length - 1];  // 高字节在前
    uint16_t received_crc_low_first = frame[length - 2] | (frame[length - 1] << 8);  // 低字节在前
    uint16_t calculated_crc = Modbus_CalculateCRC16(frame, length - 2);
    
    Log_Debug("Modbus: CRC validation - High first: 0x%04X, Low first: 0x%04X, Calculated: 0x%04X", 
              received_crc_high_first, received_crc_low_first, calculated_crc);
    
    // 检查两种字节序
    return (received_crc_high_first == calculated_crc) || (received_crc_low_first == calculated_crc);
}

/**
 * @brief 构建Modbus响应帧
 * @param slave_addr 从机地址
 * @param function_code 功能码
 * @param data 数据
 * @param data_length 数据长度
 * @param response 响应缓冲区
 * @param response_length 响应长度
 */
void Modbus_BuildResponse(uint8_t slave_addr, uint8_t function_code, uint8_t* data, uint16_t data_length, uint8_t* response, uint16_t* response_length)
{
    uint16_t pos = 0;
    
    // 从机地址
    response[pos++] = slave_addr;
    
    // 功能码
    response[pos++] = function_code;
    
    // 数据长度
    response[pos++] = data_length;
    
    // 数据
    for (uint16_t i = 0; i < data_length; i++)
    {
        response[pos++] = data[i];
    }
    
    // 计算CRC
    uint16_t crc = Modbus_CalculateCRC16(response, pos);
    response[pos++] = crc & 0xFF;           // CRC低字节在前
    response[pos++] = (crc >> 8) & 0xFF;   // CRC高字节在后
    
    *response_length = pos;
}

/**
 * @brief 构建异常响应帧
 * @param slave_addr 从机地址
 * @param function_code 功能码
 * @param exception_code 异常码
 * @param response 响应缓冲区
 * @param response_length 响应长度
 */
void Modbus_BuildExceptionResponse(uint8_t slave_addr, uint8_t function_code, uint8_t exception_code, uint8_t* response, uint16_t* response_length)
{
    uint16_t pos = 0;
    
    // 从机地址
    response[pos++] = slave_addr;
    
    // 功能码（设置最高位表示异常）
    response[pos++] = function_code | 0x80;
    
    // 异常码
    response[pos++] = exception_code;
    
    // 计算CRC
    uint16_t crc = Modbus_CalculateCRC16(response, pos);
    response[pos++] = crc & 0xFF;           // CRC低字节在前
    response[pos++] = (crc >> 8) & 0xFF;   // CRC高字节在后
    
    *response_length = pos;
}

/**
 * @brief 检测是否为Modbus命令
 * @param rx_buffer 接收缓冲区
 * @param rx_length 接收长度
 * @return true: 是Modbus命令, false: 不是Modbus命令
 */
bool Modbus_IsModbusCommand(uint8_t* rx_buffer, uint16_t rx_length)
{
    // 检查最小帧长度
    if (rx_length < 4) {
        return false;
    }
    
    // 检查从机地址（假设本设备地址为1）
    uint8_t slave_addr = rx_buffer[0];
    if (slave_addr != 0x01) {
        return false;
    }
    
    // 检查功能码是否在有效范围内
    uint8_t function_code = rx_buffer[1];
    if (function_code != MODBUS_READ_HOLDING_REGISTERS && 
        function_code != MODBUS_READ_INPUT_REGISTERS) {
        return false;
    }
    
    // 检查CRC（简单验证）
    if (!Modbus_ValidateCRC(rx_buffer, rx_length)) {
        return false;
    }
    
    return true;
}

/**
 * @brief 处理Modbus请求
 * @param rx_buffer 接收缓冲区
 * @param rx_length 接收长度
 * @param tx_buffer 发送缓冲区
 * @param tx_length 发送长度指针
 * @return true: 处理成功, false: 处理失败
 */
bool Modbus_ProcessRequest(uint8_t* rx_buffer, uint16_t rx_length, uint8_t* tx_buffer, uint16_t* tx_length)
{
//    Log_Debug("Modbus_ProcessRequest: RX length=%d", rx_length);
    
    // 检查最小帧长度
    if (rx_length < 4) {
        Log_Error("Modbus: Frame too short, length=%d", rx_length);
        return false;
    }
    
    // 验证CRC
    if (!Modbus_ValidateCRC(rx_buffer, rx_length)) {
        Log_Error("Modbus: CRC validation failed");
        return false;
    }
    
    uint8_t slave_addr = rx_buffer[0];
    uint8_t function_code = rx_buffer[1];
    
    Log_Debug("Modbus: Slave addr=0x%02X, Function=0x%02X", slave_addr, function_code);
    
    // 检查从机地址（假设本设备地址为1）
    if (slave_addr != 0x01) {
        Log_Error("Modbus: Invalid slave address 0x%02X, expected 0x01", slave_addr);
        return false;
    }
    
    switch (function_code)
    {
        case MODBUS_READ_HOLDING_REGISTERS:
        {
            // 读取保持寄存器 (功能码 0x03)
            uint16_t start_addr = (rx_buffer[2] << 8) | rx_buffer[3];
            uint16_t register_count = (rx_buffer[4] << 8) | rx_buffer[5];
            
            Log_Debug("Modbus: Read Holding Registers - Start=0x%04X, Count=%d", start_addr, register_count);
            
            // 检查地址和数量范围
            if (start_addr + register_count > MODBUS_REGISTER_COUNT)
            {
                Log_Error("Modbus: Illegal data address - Start=0x%04X, Count=%d, Max=%d", 
                         start_addr, register_count, MODBUS_REGISTER_COUNT);
                Modbus_BuildExceptionResponse(slave_addr, function_code, MODBUS_EXCEPTION_ILLEGAL_DATA_ADDR, tx_buffer, tx_length);
                return true;
            }
            
            // 构建响应数据
            uint8_t response_data[64];
            uint16_t data_pos = 0;
            
            for (uint16_t i = 0; i < register_count; i++)
            {
                uint16_t reg_addr = start_addr + i;
                uint16_t reg_value = 0;
                
                // 根据寄存器地址获取数据
                switch (reg_addr)
                {
                    case REG_PRESSURE_ADDR:
                        // 发送任务通知触发压力读取和LCD刷新，然后读取全局变量
                        {
                            if (g_PressureTaskHandle != NULL) {
                                osThreadFlagsSet(g_PressureTaskHandle, 0x01);  /* 发送压力任务通知 */
//                                Log_Debug("Modbus: Sent pressure task notification");
                            }
                            
                            if (g_LCDTaskHandle != NULL) {
                                osThreadFlagsSet(g_LCDTaskHandle, 0x01);  /* 发送LCD任务通知 */
                                Log_Info("Modbus: Sent LCD task notification");
                            } else {
                                Log_Warn("Modbus: g_LCDTaskHandle is NULL");
                            }
                            
                            /* 等待一小段时间让压力任务完成读取 */
                            osDelay(100);  /* 等待100ms */
                            
                            /* 读取最新的全局压力值 */
                            double pressure_value = PressureSensor_GetLatestValue();
                            if (pressure_value != PRESSURE_READ_ERROR)
                            {
                                // 将压力值*10000后存储到寄存器
                                uint16_t pressure_scaled = (uint16_t)(pressure_value * 10000);
                                g_modbus_registers.pressure = pressure_scaled;
//                                Log_Info("Modbus: Pressure read via task notification: %.4f MPa, scaled: %d", pressure_value, pressure_scaled);
                            }
                            else
                            {
                                Log_Warn("Modbus: Failed to get pressure value after task notification");
                            }
                        }
                        reg_value = g_modbus_registers.pressure;
                        break;
                    case REG_TEMPERATURE_ADDR:
                        // 发送任务通知触发DHT11采集和LCD刷新，然后读取全局变量
                        {
                            if (g_DHT11TaskHandle != NULL) {
                                osThreadFlagsSet(g_DHT11TaskHandle, 0x01);  /* 发送DHT11任务通知 */
                                Log_Info("Modbus: Sent DHT11 task notification");
                            } else {
                                Log_Warn("Modbus: g_DHT11TaskHandle is NULL");
                            }
                            
                            if (g_LCDTaskHandle != NULL) {
                                osThreadFlagsSet(g_LCDTaskHandle, 0x01);  /* 发送LCD任务通知 */
                                Log_Info("Modbus: Sent LCD task notification");
                            } else {
                                Log_Warn("Modbus: g_LCDTaskHandle is NULL");
                            }
                            
                            /* 等待一小段时间让DHT11任务完成读取 */
                            osDelay(100);  /* 等待100ms */
                        }
                        reg_value = g_modbus_registers.temperature;
                        break;
                    case REG_ERROR_COUNT_ADDR:
                        reg_value = g_modbus_registers.error_count;
                        break;
                    default:
                        reg_value = 0;
                        break;
                }
                
                // 高字节在前
                response_data[data_pos++] = (reg_value >> 8) & 0xFF;
                response_data[data_pos++] = reg_value & 0xFF;
            }
            
            // 构建响应帧
            Log_Debug("Modbus: Building response with %d bytes", data_pos);
            Modbus_BuildResponse(slave_addr, function_code, response_data, data_pos, tx_buffer, tx_length);
            return true;
        }
        
        case MODBUS_READ_INPUT_REGISTERS:
        {
            // 读取输入寄存器 (功能码 0x04)
            uint16_t start_addr = (rx_buffer[2] << 8) | rx_buffer[3];
            uint16_t register_count = (rx_buffer[4] << 8) | rx_buffer[5];
            
            Log_Debug("Modbus: Read Input Registers - Start=0x%04X, Count=%d", start_addr, register_count);
            
            // 检查地址和数量范围
            if (start_addr + register_count > MODBUS_REGISTER_COUNT)
            {
                Log_Error("Modbus: Illegal data address - Start=0x%04X, Count=%d, Max=%d", 
                         start_addr, register_count, MODBUS_REGISTER_COUNT);
                Modbus_BuildExceptionResponse(slave_addr, function_code, MODBUS_EXCEPTION_ILLEGAL_DATA_ADDR, tx_buffer, tx_length);
                return true;
            }
            
            // 构建响应数据（与保持寄存器相同）
            uint8_t response_data[64];
            uint16_t data_pos = 0;
 
            for (uint16_t i = 0; i < register_count; i++)
            {
                uint16_t reg_addr = start_addr + i;
                uint16_t reg_value = 0;
                
                switch (reg_addr)
                {
                    case REG_PRESSURE_ADDR:
                        // 发送任务通知触发压力读取和LCD刷新，然后读取全局变量
                        {
                            if (g_PressureTaskHandle != NULL) {
                                osThreadFlagsSet(g_PressureTaskHandle, 0x01);  /* 发送压力任务通知 */
                                Log_Debug("Modbus: Sent pressure task notification");
                            }
                            
                            if (g_LCDTaskHandle != NULL) {
                                osThreadFlagsSet(g_LCDTaskHandle, 0x01);  /* 发送LCD任务通知 */
                                Log_Debug("Modbus: Sent LCD task notification");
                            }
                            
                            /* 等待一小段时间让压力任务完成读取 */
                            osDelay(100);  /* 等待100ms */
                            
                            /* 读取最新的全局压力值 */
                            double pressure_value = PressureSensor_GetLatestValue();
                            if (pressure_value != PRESSURE_READ_ERROR)
                            {
                                // 将压力值*10000后存储到寄存器
                                uint16_t pressure_scaled = (uint16_t)(pressure_value * 10000);
                                g_modbus_registers.pressure = pressure_scaled;
                                Log_Info("Modbus: Pressure read via task notification: %.4f MPa, scaled: %d", pressure_value, pressure_scaled);
                            }
                            else
                            {
                                Log_Warn("Modbus: Failed to get pressure value after task notification");
                            }
                        }
                        reg_value = g_modbus_registers.pressure;
                        break;
                    case REG_TEMPERATURE_ADDR:
                        // 发送任务通知触发DHT11采集和LCD刷新，然后读取全局变量
                        {
                            if (g_DHT11TaskHandle != NULL) {
                                osThreadFlagsSet(g_DHT11TaskHandle, 0x01);  /* 发送DHT11任务通知 */
                                Log_Info("Modbus: Sent DHT11 task notification");
                            } else {
                                Log_Warn("Modbus: g_DHT11TaskHandle is NULL");
                            }
                            
                            if (g_LCDTaskHandle != NULL) {
                                osThreadFlagsSet(g_LCDTaskHandle, 0x01);  /* 发送LCD任务通知 */
                                Log_Info("Modbus: Sent LCD task notification");
                            } else {
                                Log_Warn("Modbus: g_LCDTaskHandle is NULL");
                            }
                            
                            /* 等待一小段时间让dht11任务完成读取 */
                            osDelay(100);  /* 等待100ms */
                            
                            /* 读取最新的全局温度值 */
                            float temp_value = TemperatureSensor_GetLatestValue();
                            
                                // 将压力值*10000后存储到寄存器
                                uint16_t temp_scaled = (uint16_t)(temp_value * 10000);
                                g_modbus_registers.temperature = temp_scaled;
                                Log_Info("Modbus: Temperature read via task notification: %.2f C, scaled: %d", temp_value, temp_scaled);
                        }
                        reg_value = g_modbus_registers.temperature;
                        break;
                    case REG_HUMIDITY_ADDR:
                        reg_value = g_modbus_registers.humidity;
                        break;
                    default:
                        reg_value = 0;
                        break;
                }
                
                response_data[data_pos++] = (reg_value >> 8) & 0xFF;
                response_data[data_pos++] = reg_value & 0xFF;
            }
            
            Log_Debug("Modbus: Building response with %d bytes", data_pos);
            Modbus_BuildResponse(slave_addr, function_code, response_data, data_pos, tx_buffer, tx_length);
            return true;
        }
        
        default:
            // 不支持的功能码
            Log_Error("Modbus: Unsupported function code 0x%02X", function_code);
            Modbus_BuildExceptionResponse(slave_addr, function_code, MODBUS_EXCEPTION_ILLEGAL_FUNCTION, tx_buffer, tx_length);
            return true;
    }
}

// ============================================================================
// 全局传感器数据管理函数
// ============================================================================

/**
 * @brief 初始化全局传感器数据
 */
void SensorData_Init(void)
{
    // 清零所有数据
    memset(&g_sensor_data, 0, sizeof(GlobalSensorData_t));
    
    // 设置初始状态
    g_sensor_data.system_status = 0x0001;  // 系统启动状态
    g_sensor_data.system_timestamp = HAL_GetTick();
    
    Log_Info("Global sensor data initialized");
}

/**
 * @brief 更新压力传感器数据
 * @param pressure_value 压力值（MPa）
 */
void SensorData_UpdatePressure(double pressure_value)
{
    g_sensor_data.pressure_value = pressure_value;
    g_sensor_data.pressure_timestamp = HAL_GetTick();
    g_sensor_data.pressure_valid = 1;
    
    // 同时更新Modbus寄存器
    uint16_t pressure_scaled = (uint16_t)(pressure_value * 10000);
    g_modbus_registers.pressure = pressure_scaled;
    
    Log_Info("Sensor data: Pressure updated to %.4f MPa", pressure_value);
}

/**
 * @brief 更新温度传感器数据
 * @param temperature_value 温度值（°C）
 */
void SensorData_UpdateTemperature(float temperature_value)
{
    g_sensor_data.temperature = temperature_value;
    g_sensor_data.temperature_valid = 1;
    
    // 同时更新Modbus寄存器
    int16_t temp_int = (int16_t)(temperature_value * 10);
    g_modbus_registers.temperature = temp_int;
    
    Log_Info("Sensor data: Temperature updated to %.2f C", temperature_value);
}


/**
 * @brief 更新湿度传感器数据
 * @param humidity_value 湿度值（%）
 */
void SensorData_UpdateHumidity(float humidity_value)
{
    g_sensor_data.humidity = humidity_value;
    g_sensor_data.humidity_valid = 1;
    
    // 同时更新Modbus寄存器
    int16_t humidity_int = (int16_t)(humidity_value * 10);
    g_modbus_registers.humidity = humidity_int;
    
    Log_Info("Sensor data: Humidity updated to %.2f%% ", humidity_value);

}


/**
 * @brief 更新系统状态
 * @param status 系统状态字
 */
void SensorData_UpdateSystemStatus(uint16_t status)
{
    g_sensor_data.system_status = status;
    g_sensor_data.system_timestamp = HAL_GetTick();
    
    // 同时更新Modbus寄存器
    g_modbus_registers.status = status;
    
    Log_Info("Sensor data: System status updated to 0x%04X", status);
}


/**
 * @brief 更新错误计数
 * @param error_count 错误计数
 */
void SensorData_UpdateErrorCount(uint16_t error_count)
{
    g_sensor_data.error_count = error_count;
    g_sensor_data.system_timestamp = HAL_GetTick();
    
    // 同时更新Modbus寄存器
    g_modbus_registers.error_count = error_count;
    
    Log_Info("Sensor data: Error count updated to %d", error_count);
}


/**
 * @brief 获取全局传感器数据指针
 * @return 全局传感器数据指针
 */
GlobalSensorData_t* SensorData_GetGlobalData(void)
{
    return &g_sensor_data;
}
