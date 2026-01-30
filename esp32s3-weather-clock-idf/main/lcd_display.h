/**
 * @file lcd_display.h
 * @brief TFT LCD显示驱动头文件
 * @author ESP-IDF Version
 */

#ifndef __LCD_DISPLAY_H
#define __LCD_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "weather_client.h"

/* 屏幕尺寸定义（竖屏方向） */
#define SCREEN_WIDTH        80
#define SCREEN_HEIGHT       160

/* 文本显示设置 */
#define LINE_HEIGHT         12      // 每行文字的高度
#define LINE_SPACING        2       // 行与行之间的间距
#define CONTENT_START_Y     5       // 内容显示的起始Y位置

/* 颜色定义（RGB565格式） */
#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_RED           0xF800
#define COLOR_GREEN         0x07E0
#define COLOR_BLUE          0x001F
#define COLOR_CYAN          0x07FF
#define COLOR_MAGENTA       0xF81F
#define COLOR_YELLOW        0xFFE0

/* 函数声明 */
void lcd_display_init(void);
void lcd_display_clear(void);
void lcd_display_set_cursor(uint16_t x, uint16_t y);
void lcd_display_set_text_color(uint16_t color);
void lcd_display_print(const char *str);
void lcd_display_print_int(int value);
void lcd_display_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void lcd_display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/* 天气显示函数 */
void lcd_display_boot_message(void);
void lcd_display_wifi_status(uint16_t y, const char *message);
void lcd_display_weather_data(const weather_data_t *data);
void lcd_display_error(const char *message);

#endif
