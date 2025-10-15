#ifndef SCREEN_CONFIG_H
#define SCREEN_CONFIG_H

/**
 * @file screen_config.h
 * @brief 统一的屏幕尺寸配置文件
 * 
 * 此文件定义了项目中所有屏幕相关的尺寸参数，避免在多个地方重复定义
 * 导致横屏竖屏搞错的问题。
 * 
 * 硬件配置：
 * - LCD控制器：ILI9341
 * - 屏幕尺寸：320x240像素
 * - 显示方向：横屏模式（320宽 x 240高）
 */

/* 屏幕分辨率定义 */
#define SCREEN_WIDTH           320     /* 屏幕宽度（像素） */
#define SCREEN_HEIGHT          240     /* 屏幕高度（像素） */

/* LVGL显示分辨率（与屏幕分辨率一致） */
#define LVGL_HOR_RES           SCREEN_WIDTH
#define LVGL_VER_RES           SCREEN_HEIGHT

/* LVGL配置文件中的分辨率定义 */
#define MY_DISP_HOR_RES        SCREEN_WIDTH
#define MY_DISP_VER_RES        SCREEN_HEIGHT

/* ILI9341 LCD驱动层定义 */
#define ILI9341_LESS_PIXEL     SCREEN_HEIGHT    /* 较短边像素数 */
#define ILI9341_MORE_PIXEL     SCREEN_WIDTH     /* 较长边像素数 */

/* 屏幕方向定义 */
typedef enum {
    SCREEN_PORTRAIT = 0,       /* 竖屏模式：240x320 */
    SCREEN_LANDSCAPE = 1       /* 横屏模式：320x240 */
} screen_orientation_t;

/* 当前屏幕方向（默认为横屏） */
#define CURRENT_SCREEN_ORIENTATION  SCREEN_LANDSCAPE

/* 根据屏幕方向获取实际的分辨率 */
#if (CURRENT_SCREEN_ORIENTATION == SCREEN_LANDSCAPE)
    #define ACTUAL_SCREEN_WIDTH    SCREEN_WIDTH
    #define ACTUAL_SCREEN_HEIGHT   SCREEN_HEIGHT
#else
    #define ACTUAL_SCREEN_WIDTH    SCREEN_HEIGHT
    #define ACTUAL_SCREEN_HEIGHT   SCREEN_WIDTH
#endif

/* 屏幕尺寸验证宏 */
#define SCREEN_SIZE_VALIDATE() \
    do { \
        static_assert(SCREEN_WIDTH == 320, "SCREEN_WIDTH must be 320"); \
        static_assert(SCREEN_HEIGHT == 240, "SCREEN_HEIGHT must be 240"); \
        static_assert(LVGL_HOR_RES == SCREEN_WIDTH, "LVGL_HOR_RES must match SCREEN_WIDTH"); \
        static_assert(LVGL_VER_RES == SCREEN_HEIGHT, "LVGL_VER_RES must match SCREEN_HEIGHT"); \
    } while(0)

#endif /* SCREEN_CONFIG_H */
