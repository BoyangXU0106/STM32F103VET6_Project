#ifndef __MODBUS_H
#define __MODBUS_H

#include "main.h"
#include <stdint.h>
#include "stdbool.h"

// Modbus功能码定义
#define MODBUS_READ_HOLDING_REGISTERS    0x03
#define MODBUS_READ_INPUT_REGISTERS      0x04
#define MODBUS_WRITE_SINGLE_REGISTER     0x06
#define MODBUS_WRITE_MULTIPLE_REGISTERS  0x10

// Modbus异常码定义
#define MODBUS_EXCEPTION_ILLEGAL_FUNCTION    0x01
#define MODBUS_EXCEPTION_ILLEGAL_DATA_ADDR   0x02
#define MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE  0x03
#define MODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE 0x04

// Modbus寄存器地址定义
#define MODBUS_REG_PRESSURE_ADDR     0x00C8  // 压力值寄存器地址
#define MODBUS_REG_TEMPERATURE_ADDR  0x00c9  // 温度值寄存器地址
#define MODBUS_REG_STATUS_ADDR       0x00ca  // 状态寄存器地址
#define MODBUS_REG_ERROR_COUNT_ADDR  0x0003  // 错误计数寄存器地址

// 兼容性别名
#define REG_PRESSURE_ADDR            MODBUS_REG_PRESSURE_ADDR
#define REG_TEMPERATURE_ADDR         MODBUS_REG_TEMPERATURE_ADDR
#define REG_STATUS_ADDR              MODBUS_REG_STATUS_ADDR
#define REG_ERROR_COUNT_ADDR         MODBUS_REG_ERROR_COUNT_ADDR

// 寄存器数量定义
#define MODBUS_REGISTER_COUNT        300       // 总寄存器数量

// Modbus寄存器结构体
typedef struct {
    uint16_t pressure;      // 压力值 (0.1 kPa)
    uint16_t temperature;   // 温度值 (0.1 °C)
    uint16_t status;        // 状态字
    uint16_t error_count;   // 错误计数
} ModbusRegisters_t;

// 全局变量声明
extern ModbusRegisters_t g_modbus_registers;

// 函数声明
void Modbus_Init(void);
void Modbus_UpdatePressure(float pressure_value);
void Modbus_UpdateTemperature(float temperature_value);
void Modbus_UpdateStatus(uint16_t status_value);
bool Modbus_ProcessRequest(uint8_t* request, uint16_t request_length, uint8_t* response, uint16_t* response_length);
void Modbus_BuildResponse(uint8_t slave_addr, uint8_t function_code, uint8_t* data, uint16_t data_length, uint8_t* response, uint16_t* response_length);
void Modbus_BuildExceptionResponse(uint8_t slave_addr, uint8_t function_code, uint8_t exception_code, uint8_t* response, uint16_t* response_length);
uint16_t Modbus_CalculateCRC16(uint8_t* data, uint16_t length);

#endif /* __MODBUS_H */
