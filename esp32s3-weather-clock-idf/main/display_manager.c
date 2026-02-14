/**
 * @file display_manager.c
 * @brief 显示管理模块实现
 * @author ESP-IDF Version
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd.h"
#include "ntp_client.h"
#include "display_manager.h"

/* 外部声明字体数组 */
extern const unsigned char asc2_1206[95][12];
extern const unsigned char asc2_1608[95][16];
extern const unsigned char asc2_2412[95][36];
extern const unsigned char asc2_3216[95][64];

static const char *TAG = "DISPLAY_MANAGER";

/* 显示消息行计数器 */
uint8_t display_manager_current_line = 0;

/* 全局数据 */
static weather_data_t s_current_weather_data = {0};
static bool s_weather_data_available = false;

/* 保存上一次的时间，用于检测变化 */
static int s_last_hour = -1;
static int s_last_minute = -1;
static int s_last_second = -1;
static int s_last_year = -1;
static int s_last_month = -1;
static int s_last_day = -1;

/**
 * @brief 将中文天气状况转换为英文
 */
static const char* translate_weather(const char* condition)
{
    if (condition == NULL) return "N/A";
    
    if (strcmp(condition, "晴") == 0) return "SUNNY";
    if (strcmp(condition, "多云") == 0) return "CLOUDY";
    if (strcmp(condition, "阴") == 0) return "OVERCAST";
    if (strcmp(condition, "雨") == 0) return "RAIN";
    if (strcmp(condition, "雪") == 0) return "SNOW";
    if (strcmp(condition, "雾") == 0) return "FOG";
    
    return condition; /* 如果没有匹配的，返回原始值 */
}

/**
 * @brief 将中文风向转换为英文缩写
 */
static const char* translate_wind_dir(const char* wind_dir)
{
    if (wind_dir == NULL) return "N/A";
    
    /* 带风字的方向 */
    if (strcmp(wind_dir, "北风") == 0) return "N";
    if (strcmp(wind_dir, "东北风") == 0) return "NE";
    if (strcmp(wind_dir, "东风") == 0) return "E";
    if (strcmp(wind_dir, "东南风") == 0) return "SE";
    if (strcmp(wind_dir, "南风") == 0) return "S";
    if (strcmp(wind_dir, "西南风") == 0) return "SW";
    if (strcmp(wind_dir, "西风") == 0) return "W";
    if (strcmp(wind_dir, "西北风") == 0) return "NW";
    
    /* 如果没有匹配的，记录日志并返回原始值 */
    ESP_LOGW(TAG, "Unknown wind direction: %s", wind_dir);
    return wind_dir;
}

/**
 * @brief 显示字符（黑色背景）
 * @param x,y     起始坐标
 * @param chr     字符
 * @param size    字体大小
 * @param color   字体颜色
 */
static void lcd_show_char_black_bg(uint16_t x, uint16_t y, uint8_t chr, uint8_t size, uint16_t color)
{
    uint8_t temp = 0, t1 = 0, t = 0;
    uint8_t *pfont = 0;
    uint8_t csize = 0;
    uint16_t colortemp = 0;
    uint8_t sta = 0;

    csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);
    chr = chr - ' ';

    if ((x > (79 - size / 2)) || (y > (159 - size)))
    {
        return;
    }

    lcd_set_window(x, y, x + size / 2 - 1, y + size - 1);

    switch (size)
    {
        case 12:
            pfont = (uint8_t *)asc2_1206[chr];
            break;

        case 16:
            pfont = (uint8_t *)asc2_1608[chr];
            break;

        case 24:
            pfont = (uint8_t *)asc2_2412[chr];
            break;

        case 32:
            pfont = (uint8_t *)asc2_3216[chr];
            break;

        default:
            return;
    }

    if (size != 24)
    {
        csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);
        uint8_t pixels_per_line = size / 2;
        
        for (t = 0; t < csize; t++)
        {
            temp = pfont[t];

            for (t1 = 0; t1 < pixels_per_line; t1++)
            {
                if (temp & 0x80)
                {
                    colortemp = color;
                }
                else
                {
                    colortemp = 0x0000;  /* 黑色背景 */
                }

                lcd_write_data16(colortemp);
                temp <<= 1;
            }
        }
    }
    else
    {
        csize = (size * 16) / 8;
        
        for (t = 0; t < csize; t++)
        {
            temp = asc2_2412[chr][t];

            if (t % 2 == 0)
            {
                sta = 8;
            }
            else
            {
                sta = 4;
            }

            for (t1 = 0; t1 < sta; t1++)
            {
                if (temp & 0x80)
                {
                    colortemp = color;
                }
                else
                {
                    colortemp = 0x0000;  /* 黑色背景 */
                }

                lcd_write_data16(colortemp);
                temp <<= 1;
            }
        }
    }
}

/**
 * @brief 显示没有背景的字符串
 * @param x,y    起始坐标
 * @param width   宽度
 * @param height  高度
 * @param size    字体大小
 * @param p       字符串指针
 * @param color   字体颜色
 */
static void lcd_show_string_no_bg(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
    uint8_t x0 = x;
    width += x;
    height += y;

    while ((*p <= '~') && (*p >= ' '))
    {
        if (x >= width)
        {
            x = x0;
            y += size;
        }

        if (y >= height) break;

        /* 使用mode=0来显示清晰的字体（黑色背景） */
        lcd_show_char_black_bg(x, y, *p, size, color);
        x += size / 2;
        p++;
    }
}

/**
 * @brief 初始化显示管理器
 */
esp_err_t display_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing display manager");
    
    /* 初始化SPI */
    ESP_LOGD(TAG, "Initializing SPI2");
    spi2_init();
    
    /* 初始化LCD */
    ESP_LOGD(TAG, "Initializing LCD");
    lcd_init();
    
    /* 设置为竖屏模式 */
    ESP_LOGD(TAG, "Setting LCD to portrait mode");
    extern void lcd_display_dir(uint8_t dir);
    lcd_display_dir(0);
    
    /* 清屏 */
    ESP_LOGD(TAG, "Clearing LCD screen");
    lcd_fill(0, 0, 79, 159, BLACK);
    
    /* 显示初始化信息 */
    ESP_LOGD(TAG, "Showing initialization message");
/* 显示初始化消息 */
    lcd_show_string_no_bg(0, 0, 80, 16, 16, "Weather Clock", WHITE);
    lcd_show_string_no_bg(0, 20, 80, 16, 16, "Initializing...", WHITE);
    
    /* 初始化时间状态 */
    s_last_hour = -1;
    s_last_minute = -1;
    s_last_second = -1;
    s_last_year = -1;
    s_last_month = -1;
    s_last_day = -1;
    
    ESP_LOGI(TAG, "Display manager initialized successfully");
    return ESP_OK;
}

/**
 * @brief 只更新时间显示（不刷新整个屏幕）
 */
esp_err_t display_manager_update_time(void)
{
    ESP_LOGD(TAG, "Updating time display only");
    
    /* 显示时间信息 */
    ESP_LOGV(TAG, "Displaying time information");
    struct tm timeinfo;
    if (ntp_client_get_time(&timeinfo)) {
        int current_hour = timeinfo.tm_hour;
        int current_minute = timeinfo.tm_min;
        int current_second = timeinfo.tm_sec;
        int current_year = timeinfo.tm_year;
        int current_month = timeinfo.tm_mon;
        int current_day = timeinfo.tm_mday;
        
        /* 检查日期变化 */
        if (current_year != s_last_year || current_month != s_last_month || current_day != s_last_day) {
            /* 只清除日期区域（y=0, 高度=16） */
            ESP_LOGV(TAG, "Updating date: %04d-%02d-%02d", current_year + 1900, current_month + 1, current_day);
            lcd_fill(0, 0, 79, 15, BLACK);
            
            /* 显示日期 */
            char date_str[32];
            strftime(date_str, sizeof(date_str), "%y-%m-%d", &timeinfo);
            lcd_show_string_no_bg(0, 0, 80, 16, 16, date_str, WHITE);
            
            s_last_year = current_year;
            s_last_month = current_month;
            s_last_day = current_day;
        }
        
        /* 检查秒数变化 */
        if (current_second != s_last_second) {
            /* 只清除秒数区域（时间格式HH:MM:SS，秒数从索引6开始，每个字符8像素） */
            /* 秒数位置：x=48 (6*8), y=20, 宽度=16 (2*8) */
            ESP_LOGV(TAG, "Updating seconds: %02d", current_second);
            lcd_fill(48, 20, 63, 35, BLACK);
            
            /* 只显示秒数部分 */
            char seconds_str[8];
            snprintf(seconds_str, sizeof(seconds_str), "%02d", current_second);
            lcd_show_string_no_bg(48, 20, 80, 16, 16, seconds_str, WHITE);
            
            /* 确保显示分钟和秒之间的冒号 */
            lcd_show_string_no_bg(40, 20, 80, 16, 16, ":", WHITE);
            
            s_last_second = current_second;
        }
        
        /* 检查分钟变化 */
        if (current_minute != s_last_minute) {
            /* 只清除分钟区域（时间格式HH:MM:SS，分钟从索引3开始，每个字符8像素） */
            /* 分钟位置：x=24 (3*8), y=20, 宽度=16 (2*8) */
            ESP_LOGV(TAG, "Updating minutes: %02d", current_minute);
            lcd_fill(24, 20, 39, 35, BLACK);
            
            /* 只显示分钟部分 */
            char minutes_str[8];
            snprintf(minutes_str, sizeof(minutes_str), "%02d", current_minute);
            lcd_show_string_no_bg(24, 20, 80, 16, 16, minutes_str, WHITE);
            
            /* 确保显示分钟和秒之间的冒号 */
            lcd_show_string_no_bg(40, 20, 80, 16, 16, ":", WHITE);
            
            s_last_minute = current_minute;
        }
        
        /* 检查小时变化 */
        if (current_hour != s_last_hour) {
            /* 只清除小时区域（时间格式HH:MM:SS，小时从索引0开始，每个字符8像素） */
            /* 小时位置：x=0, y=20, 宽度=16 (2*8) */
            ESP_LOGV(TAG, "Updating hours: %02d", current_hour);
            lcd_fill(0, 20, 15, 35, BLACK);
            
            /* 只显示小时部分 */
            char hours_str[8];
            snprintf(hours_str, sizeof(hours_str), "%02d", current_hour);
            lcd_show_string_no_bg(0, 20, 80, 16, 16, hours_str, WHITE);
            
            /* 确保显示小时和分钟之间的冒号 */
            lcd_show_string_no_bg(16, 20, 80, 16, 16, ":", WHITE);
            
            s_last_hour = current_hour;
        }
    } else {
        ESP_LOGW(TAG, "NTP time not available, showing placeholder");
        lcd_show_string_no_bg(0, 0, 80, 16, 16, "--:--:--", WHITE);
    }
    
    ESP_LOGD(TAG, "Time display updated successfully");
    return ESP_OK;
}

/**
 * @brief 更新显示
 */
esp_err_t display_manager_update(void)
{
    ESP_LOGD(TAG, "Updating display");
    
    /* 清屏 */
    ESP_LOGV(TAG, "Clearing LCD screen");
    lcd_fill(0, 0, 79, 159, BLACK);
    
    /* 显示时间信息 */
    ESP_LOGV(TAG, "Displaying time information");
    struct tm timeinfo;
    if (ntp_client_get_time(&timeinfo)) {
        /* 显示日期（第一行） */
        char date_str[32];
        strftime(date_str, sizeof(date_str), "%y-%m-%d", &timeinfo);
        ESP_LOGV(TAG, "Date: %s", date_str);
        lcd_show_string_no_bg(0, 0, 80, 16, 16, date_str, WHITE);
        
        /* 显示时间（第二行） */
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);
        ESP_LOGV(TAG, "Time: %s", time_str);
        lcd_show_string_no_bg(0, 20, 80, 16, 16, time_str, WHITE);
        
        /* 更新时间状态 */
        s_last_hour = timeinfo.tm_hour;
        s_last_minute = timeinfo.tm_min;
        s_last_second = timeinfo.tm_sec;
        s_last_year = timeinfo.tm_year;
        s_last_month = timeinfo.tm_mon;
        s_last_day = timeinfo.tm_mday;
    } else {
        ESP_LOGW(TAG, "NTP time not available, showing placeholder");
        /* 显示日期占位符 */
        lcd_show_string_no_bg(0, 0, 80, 16, 16, "----", WHITE);
        /* 显示时间占位符 */
        lcd_show_string_no_bg(0, 20, 80, 16, 16, "--:--:--", WHITE);
    }
    
    /* 显示天气数据 */
    if (s_weather_data_available) {
        ESP_LOGV(TAG, "Displaying weather information");
        
        /* 天气状况（第三行） */
        const char* weather_en = translate_weather(s_current_weather_data.text);
        ESP_LOGV(TAG, "Weather condition: %s", weather_en);
        lcd_show_string_no_bg(0, 40, 80, 16, 16, "WX:", YELLOW);
        char weather_copy[16];
        strncpy(weather_copy, weather_en, sizeof(weather_copy) - 1);
        weather_copy[sizeof(weather_copy) - 1] = '\0';
        lcd_show_string_no_bg(40, 40, 80, 16, 16, weather_copy, BLUE);
        
        /* 温度（第四行） */
        char temp_str[16];
        snprintf(temp_str, sizeof(temp_str), "%sC", s_current_weather_data.temp);
        ESP_LOGV(TAG, "Temperature: %s", temp_str);
        lcd_show_string_no_bg(0, 60, 80, 16, 16, "Temp:", YELLOW);
        lcd_show_string_no_bg(40, 60, 80, 16, 16, temp_str, BLUE);
        
        /* 体感温度 */
        char feels_like_str[16];
        snprintf(feels_like_str, sizeof(feels_like_str), "%sC", s_current_weather_data.feelsLike);
        ESP_LOGV(TAG, "Feels like: %s", feels_like_str);
        lcd_show_string_no_bg(0, 80, 80, 16, 16, "Feel:", YELLOW);
        lcd_show_string_no_bg(40, 80, 80, 16, 16, feels_like_str, BLUE);
        
        /* 风向和风力 */
        const char* wind_dir_en = translate_wind_dir(s_current_weather_data.windDir);
        ESP_LOGV(TAG, "Wind direction: %s, scale: %s", wind_dir_en, s_current_weather_data.windScale);
        lcd_show_string_no_bg(0, 100, 80, 16, 16, "Wind:", YELLOW);
        char wind_copy[8];
        strncpy(wind_copy, wind_dir_en, sizeof(wind_copy) - 1);
        wind_copy[sizeof(wind_copy) - 1] = '\0';
        lcd_show_string_no_bg(40, 100, 80, 16, 16, wind_copy, BLUE);
        lcd_show_string_no_bg(0, 120, 80, 16, 16, "Spd:", YELLOW);
        lcd_show_string_no_bg(40, 120, 80, 16, 16, s_current_weather_data.windScale, BLUE);
        
        /* 湿度 */
        char humidity_str[16];
        snprintf(humidity_str, sizeof(humidity_str), "%s%%", s_current_weather_data.humidity);
        ESP_LOGV(TAG, "Humidity: %s", humidity_str);
        lcd_show_string_no_bg(0, 140, 80, 16, 16, "Hum:", YELLOW);
        lcd_show_string_no_bg(40, 140, 80, 16, 16, humidity_str, BLUE);
    } else {
        ESP_LOGV(TAG, "No weather data available");
        lcd_show_string_no_bg(0, 40, 80, 16, 16, "No Weather Data", BLUE);
    }
    
    ESP_LOGD(TAG, "Display updated successfully");
    return ESP_OK;
}

/**
 * @brief 更新天气信息显示
 */
esp_err_t display_manager_update_weather(const weather_data_t *weather_data)
{
    if (weather_data == NULL) {
        ESP_LOGE(TAG, "Invalid weather data");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "Updating weather display");
    
    /* 复制天气数据 */
    memcpy(&s_current_weather_data, weather_data, sizeof(weather_data_t));
    s_weather_data_available = true;
    
    /* 重置时间状态，确保下次更新时显示完整时间 */
    s_last_hour = -1;
    s_last_minute = -1;
    s_last_second = -1;
    s_last_year = -1;
    s_last_month = -1;
    s_last_day = -1;
    
    /* 更新显示 */
    return display_manager_update();
}

/**
 * @brief 显示消息
 */
esp_err_t display_manager_show_message(const char *message, uint16_t color)
{
    if (message == NULL) {
        ESP_LOGE(TAG, "Invalid message");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "Showing message: %s", message);
    
    /* 显示消息（需要转换为非const指针） */
    char msg_copy[128];
    strncpy(msg_copy, message, sizeof(msg_copy) - 1);
    msg_copy[sizeof(msg_copy) - 1] = '\0';
    
    /* 在当前行显示消息 */
    uint16_t y = display_manager_current_line * 16;
    lcd_show_string_no_bg(0, y, 80, 16, 16, msg_copy, color);
    
    /* 移动到下一行 */
    display_manager_current_line++;
    
    /* 如果超出屏幕，重置到第一行 */
    if (display_manager_current_line >= 10) {
        display_manager_current_line = 0;
    }
    
    return ESP_OK;
}

/**
 * @brief 清除显示消息区域
 */
esp_err_t display_manager_clear_messages(void)
{
    ESP_LOGD(TAG, "Clearing message display");
    
    /* 清屏 */
    lcd_fill(0, 0, 79, 159, BLACK);
    
    /* 重置显示行计数器 */
    display_manager_current_line = 0;
    
    return ESP_OK;
}
