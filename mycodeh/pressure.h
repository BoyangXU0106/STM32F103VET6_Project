/* pressure.h */
#ifndef __PRESSURE_H
#define __PRESSURE_H

#include "main.h"

// �豸��ַ (7λ��ַ)
#define PRESSURE_SENSOR_ADDR    0x6D << 1  // HAL��ʹ��8λ��ַ��ʽ

// �Ĵ�������
#define CMD_REGISTER            0x30
#define DATA_REGISTER           0x06
#define PRESSURE_DATA_SIZE      3
#define PRESSURE_SCALE_B        (100000.0F)

//ѡ���Ƿ�ʹ��modbus��ѹ��ֵ
#define Modbus_pressure 1

// ��ʼ��״̬ö��
typedef enum {
    PRESSURE_OK = 0,
    PRESSURE_INIT_ERROR,
    PRESSURE_READ_ERROR
} Pressure_Status;

extern volatile uint8_t pressure_measure_flag;



// ��������
Pressure_Status PressureSensor_Init(I2C_HandleTypeDef *hi2c);
//Pressure_Status PressureSensor_ReadData(I2C_HandleTypeDef *hi2c, double *pressure);
double PressureSensor_ReadData(I2C_HandleTypeDef *hi2c);

// 新增函数声明
double PressureSensor_ReadImmediate(void);
double PressureSensor_GetLatestValue(void);
uint32_t PressureSensor_GetTimestamp(void);
void PressureSensor_UpdateValue(double pressure_value);

#if Modbus_pressure

void PressureSensor_PrintData(UART_HandleTypeDef *huart, int pressure);
int PressureGet(void);

#else

void PressureSensor_PrintData(UART_HandleTypeDef *huart, float pressure);
float PressureGet(void);

#endif



#endif /* __PRESSURE_H */
