#ifndef APP_LVGL_H
#define APP_LVGL_H

#include "stm32f1xx.h"
#include "screen_config.h"

void app_lvgl_init(void);
void app_lvgl_create_ui(void);
void app_lvgl_task(void *argument);


// ---------------- LVGL Port (display + touch) ----------------
// ���� ILI9341 (RGB565) �� XPT2046 ����
// ��ֲ��תΪ FreeRTOS ���������� LVGL

// ��ʾ�ֱ��ʣ��� ILI9341_GramScan һ�£����� 320x240 ������
//#define LVGL_HOR_RES    LVGL_HOR_RES
//#define LVGL_VER_RES    LVGL_VER_RES

// ���ײ�δ��ͷ�ļ���¶�Ĵ���д�ӿڣ�����������������ʽ����
extern void ILI9341_Write_Cmd(uint16_t usCmd);
extern void ILI9341_Write_Data(uint16_t usData);

// LVGL ��ʼ���� UI ��Ǩ�Ƶ� app_lvgl ģ��

// ---------------- ���ֻ水ť��ʾ������ LVGL ����� ----------------
// ��ť�ߴ���λ�ã�������Ļ�ߴ綯̬���У�
static uint16_t buttonX = 0;
static uint16_t buttonY = 0;
static const uint16_t buttonWidth = 120;
static const uint16_t buttonHeight = 50;

#endif /* APP_LVGL_H */

