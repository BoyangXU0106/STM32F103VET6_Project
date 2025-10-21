/* pressure.c */
#include "pressure.h"
#include <string.h>
#include "stdio.h"
#include "math.h"
#include "log.h"
#include "FreeRTOS.h"

static I2C_HandleTypeDef *hi2c_pressure;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
int16_t pressure_B;
//uint16_t pressure_value = 0;  //��Ϊת�������mobus���������
uint16_t pressure_bvalue = 0;
volatile uint8_t pressure_measure_flag = 0;    //ѹ��������������־����LPTIM��ʱ���и�ֵ

//ѹ����������ʼ��
Pressure_Status PressureSensor_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t cmd[2];
    HAL_StatusTypeDef hal_status;
    
    hi2c_pressure = hi2c;

    cmd[0] = CMD_REGISTER;
    cmd[1] = 0x00;
    hal_status = HAL_I2C_Master_Transmit(hi2c_pressure, PRESSURE_SENSOR_ADDR, cmd, 2, 100);
    if(hal_status != HAL_OK) return PRESSURE_INIT_ERROR;

    HAL_Delay(10);


    cmd[0] = CMD_REGISTER;
    cmd[1] = 0x03;
    hal_status = HAL_I2C_Master_Transmit(hi2c_pressure, PRESSURE_SENSOR_ADDR, cmd, 2, 100);
    if(hal_status != HAL_OK) return PRESSURE_INIT_ERROR;

    osDelay(50); 

    return PRESSURE_OK;
}



double PressureSensor_ReadData(I2C_HandleTypeDef *hi2c)
{
    uint8_t data[PRESSURE_DATA_SIZE];
    HAL_StatusTypeDef hal_status;
    
    hal_status = HAL_I2C_Mem_Read(hi2c_pressure, PRESSURE_SENSOR_ADDR, 
                                DATA_REGISTER, I2C_MEMADD_SIZE_8BIT,
                                data, PRESSURE_DATA_SIZE, 100);
    if(hal_status != HAL_OK) return PRESSURE_READ_ERROR;

    int32_t raw_value = ((int32_t)data[0] << 16) | ((int32_t)data[1] << 8) | data[2];
	if ((raw_value &0x800000) == 0x800000){
		raw_value = ~(raw_value - 1);
		raw_value *= -1;
	}
    
	
	Log_Info("Pressure raw value: %d", raw_value);
	
	// 应用压力值转换公式: 实际压力值 = (raw_value - 8388608 * 0.10311) * 0.8
	double actual_pressure = ((1/(0.8 * 8388608)) * (raw_value - (8388608 * 0.1)));
	
    return actual_pressure;
}




