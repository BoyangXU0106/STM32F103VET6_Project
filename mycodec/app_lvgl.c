#include "app_lvgl.h"
#include "pressure_task.h"
#include "lvgl.h"
#include "bsp_ili9341_lcd.h"
#include "bsp_xpt2046_lcd.h"
#include "cmsis_os.h"
#include "gui_guider.h"
#include "events_init.h"
#include "stdio.h"


// 某些底层符号未在头文件暴露，这里声明以消除隐式声明
extern void ILI9341_Write_Cmd(uint16_t usCmd);
extern void ILI9341_Write_Data(uint16_t usData);
extern void ILI9341_OpenWindow(uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight);
extern void ILI9341_Clear(uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight);


lv_ui guider_ui;


// ILI9341 写像素命令（在 ILI9341 驱动中定义）
#ifndef CMD_SetPixel
#define CMD_SetPixel 0x2C
#endif

static void lv_port_disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, const lv_color_t *color_p)
{
	(void)drv;
	int32_t x1 = area->x1;
	int32_t y1 = area->y1;
	int32_t w  = area->x2 - area->x1 + 1;
	int32_t h  = area->y2 - area->y1 + 1;
	if(w <= 0 || h <= 0) {
		lv_disp_flush_ready(drv);
		return;
	}

	ILI9341_OpenWindow((uint16_t)x1, (uint16_t)y1, (uint16_t)w, (uint16_t)h);
	ILI9341_Write_Cmd(CMD_SetPixel);
	uint32_t px_count = (uint32_t)w * (uint32_t)h;
	for(uint32_t i = 0; i < px_count; i++) {
		ILI9341_Write_Data(color_p[i].full);
	}

	lv_disp_flush_ready(drv);
}

void lv_port_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
	(void)indev_drv;
	strType_XPT2046_Coordinate touch = {0};
	uint8_t pressed = XPT2046_TouchDetect();
	if(pressed == TOUCH_PRESSED && XPT2046_Get_TouchedPoint(&touch, strXPT2046_TouchPara) == TOUCH_PRESSED) {
		data->state = LV_INDEV_STATE_PRESSED;
		data->point.x = (lv_coord_t)touch.x;
		data->point.y = (lv_coord_t)touch.y;
	} else {
		data->state = LV_INDEV_STATE_RELEASED;
	}
}


void app_lvgl_init(void)
{
	// 初始化 LVGL
	lv_init();

	// 注册显示驱动（单缓冲，行块大小 10 行）
	static lv_color_t draw_buf1[LVGL_HOR_RES * 10];
	static lv_disp_draw_buf_t disp_buf;
	lv_disp_draw_buf_init(&disp_buf, draw_buf1, NULL, LVGL_HOR_RES * 10);

	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.hor_res = LVGL_HOR_RES;
	disp_drv.ver_res = LVGL_VER_RES;
	disp_drv.flush_cb = lv_port_disp_flush;
	disp_drv.draw_buf = &disp_buf;
	(void)lv_disp_drv_register(&disp_drv);

	// 注册触摸输入设备
	static lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = lv_port_touch_read;
	(void)lv_indev_drv_register(&indev_drv);
}

void app_lvgl_task(void *argument)
{
	(void)argument;
	// 清屏并创建 UI
	ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);
  //	app_lvgl_create_ui();
  setup_ui(&guider_ui);
	events_init(&guider_ui);
	
	// 添加任务启动延迟，确保系统稳定
	osDelay(100);
	
	// 压力数据相关变量
	pressure_data_t pressure_data;
	char pressure_text[32];
	uint32_t last_update_time = 0;
	const uint32_t UPDATE_INTERVAL = 1000; // 1秒检查一次队列
	bool has_valid_data = false; // 标记是否有有效数据显示
	
	// 初始化显示
	snprintf(pressure_text, sizeof(pressure_text), "Pressure: Initializing...");
	lv_label_set_text(guider_ui.screen_label_1, pressure_text);

	for(;;) {
		lv_tick_inc(5);  // 5ms tick
		lv_timer_handler();
		
		// 检查是否有新的压力数据
		uint32_t current_time = osKernelGetTickCount();
		if (current_time - last_update_time >= UPDATE_INTERVAL) {
			osStatus_t queue_status = osMessageQueueGet(pressureQueueHandle, &pressure_data, NULL, 0);
			if (queue_status == osOK) {
				// 成功获取压力数据，更新显示
				if (pressure_data.valid) {
					snprintf(pressure_text, sizeof(pressure_text), "Pressure: %.2f MPa", pressure_data.pressure_value);
					has_valid_data = true; // 标记有有效数据
				} else {
					snprintf(pressure_text, sizeof(pressure_text), "Pressure: ERROR");
					has_valid_data = false;
				}
				lv_label_set_text(guider_ui.screen_label_1, pressure_text);
			} else {
				// 没有新数据，但保持当前显示不变
				// 只有在没有有效数据时才显示等待状态
				if (!has_valid_data) {
					snprintf(pressure_text, sizeof(pressure_text), "Pressure: Waiting...");
					lv_label_set_text(guider_ui.screen_label_1, pressure_text);
				}
				// 如果有有效数据，保持当前显示不变
			}
			last_update_time = current_time;
		}
		
		osDelay(5);  // 5ms延迟，与tick匹配
	}
}


