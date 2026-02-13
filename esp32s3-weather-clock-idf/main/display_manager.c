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

static const char *TAG = "DISPLAY_MANAGER";

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
    lcd_fill(0, 0, 79, 159, WHITE);
    
    /* 显示初始化信息 */
    ESP_LOGD(TAG, "Showing initialization message");
    lcd_show_string(0, 0, 150, 16, 16, "Weather Clock", BLACK);
    lcd_show_string(0, 20, 150, 16, 16, "Initializing...", BLACK);
    
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
            /* 只清除日期区域（y=20, 高度=16） */
            ESP_LOGV(TAG, "Updating date: %04d-%02d-%02d", current_year + 1900, current_month + 1, current_day);
            lcd_fill(0, 20, 79, 35, WHITE);
            
            /* 显示日期 */
            char date_str[32];
            strftime(date_str, sizeof(date_str), "%Y-%m-%d", &timeinfo);
            lcd_show_string(0, 20, 150, 16, 16, date_str, BLACK);
            
            s_last_year = current_year;
            s_last_month = current_month;
            s_last_day = current_day;
        }
        
        /* 检查秒数变化 */
        if (current_second != s_last_second) {
            /* 只清除秒数区域（时间格式HH:MM:SS，秒数从索引6开始，每个字符8像素） */
            /* 秒数位置：x=48 (6*8), 宽度=16 (2*8) */
            ESP_LOGV(TAG, "Updating seconds: %02d", current_second);
            lcd_fill(48, 0, 63, 15, WHITE);
            
            /* 只显示秒数部分 */
            char seconds_str[8];
            snprintf(seconds_str, sizeof(seconds_str), "%02d", current_second);
            lcd_show_string(48, 0, 150, 16, 16, seconds_str, BLACK);
            
            /* 确保显示分钟和秒之间的冒号 */
            lcd_show_string(40, 0, 150, 16, 16, ":", BLACK);
            
            s_last_second = current_second;
        }
        
        /* 检查分钟变化 */
        if (current_minute != s_last_minute) {
            /* 只清除分钟区域（时间格式HH:MM:SS，分钟从索引3开始，每个字符8像素） */
            /* 分钟位置：x=24 (3*8), 宽度=16 (2*8) */
            ESP_LOGV(TAG, "Updating minutes: %02d", current_minute);
            lcd_fill(24, 0, 39, 15, WHITE);
            
            /* 只显示分钟部分 */
            char minutes_str[8];
            snprintf(minutes_str, sizeof(minutes_str), "%02d", current_minute);
            lcd_show_string(24, 0, 150, 16, 16, minutes_str, BLACK);
            
            /* 确保显示分钟和秒之间的冒号 */
            lcd_show_string(40, 0, 150, 16, 16, ":", BLACK);
            
            s_last_minute = current_minute;
        }
        
        /* 检查小时变化 */
        if (current_hour != s_last_hour) {
            /* 只清除小时区域（时间格式HH:MM:SS，小时从索引0开始，每个字符8像素） */
            /* 小时位置：x=0, 宽度=16 (2*8) */
            ESP_LOGV(TAG, "Updating hours: %02d", current_hour);
            lcd_fill(0, 0, 15, 15, WHITE);
            
            /* 只显示小时部分 */
            char hours_str[8];
            snprintf(hours_str, sizeof(hours_str), "%02d", current_hour);
            lcd_show_string(0, 0, 150, 16, 16, hours_str, BLACK);
            
            /* 确保显示小时和分钟之间的冒号 */
            lcd_show_string(16, 0, 150, 16, 16, ":", BLACK);
            
            s_last_hour = current_hour;
        }
    } else {
        ESP_LOGW(TAG, "NTP time not available, showing placeholder");
        lcd_show_string(0, 0, 150, 16, 16, "--:--:--", BLACK);
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
    lcd_fill(0, 0, 79, 159, WHITE);
    
    /* 显示时间信息 */
    ESP_LOGV(TAG, "Displaying time information");
    struct tm timeinfo;
    if (ntp_client_get_time(&timeinfo)) {
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);
        ESP_LOGV(TAG, "Time: %s", time_str);
        lcd_show_string(0, 0, 150, 16, 16, time_str, BLACK);
        
        char date_str[32];
        strftime(date_str, sizeof(date_str), "%Y-%m-%d", &timeinfo);
        ESP_LOGV(TAG, "Date: %s", date_str);
        lcd_show_string(0, 20, 150, 16, 16, date_str, BLACK);
        
        /* 更新时间状态 */
        s_last_hour = timeinfo.tm_hour;
        s_last_minute = timeinfo.tm_min;
        s_last_second = timeinfo.tm_sec;
        s_last_year = timeinfo.tm_year;
        s_last_month = timeinfo.tm_mon;
        s_last_day = timeinfo.tm_mday;
    } else {
        ESP_LOGW(TAG, "NTP time not available, showing placeholder");
        lcd_show_string(0, 0, 150, 16, 16, "--:--:--", BLACK);
    }
    
    /* 显示天气数据 */
    if (s_weather_data_available) {
        ESP_LOGV(TAG, "Displaying weather information");
        
        /* 温度 */
        char temp_str[16];
        snprintf(temp_str, sizeof(temp_str), "%sC", s_current_weather_data.temp);
        ESP_LOGV(TAG, "Temperature: %s", temp_str);
        lcd_show_string(0, 40, 150, 16, 16, "Temp:", BLUE);
        lcd_show_string(40, 40, 150, 16, 16, temp_str, RED);
        
        /* 天气状况 */
        const char* weather_en = translate_weather(s_current_weather_data.text);
        ESP_LOGV(TAG, "Weather condition: %s", weather_en);
        lcd_show_string(0, 60, 150, 16, 16, "WX:", BLUE);
        char weather_copy[16];
        strncpy(weather_copy, weather_en, sizeof(weather_copy) - 1);
        weather_copy[sizeof(weather_copy) - 1] = '\0';
        lcd_show_string(40, 60, 150, 16, 16, weather_copy, RED);
        
        /* 体感温度 */
        char feels_like_str[16];
        snprintf(feels_like_str, sizeof(feels_like_str), "%sC", s_current_weather_data.feelsLike);
        ESP_LOGV(TAG, "Feels like: %s", feels_like_str);
        lcd_show_string(0, 80, 150, 16, 16, "Feel:", BLUE);
        lcd_show_string(40, 80, 150, 16, 16, feels_like_str, RED);
        
        /* 风向和风力 */
        const char* wind_dir_en = translate_wind_dir(s_current_weather_data.windDir);
        ESP_LOGV(TAG, "Wind direction: %s, scale: %s", wind_dir_en, s_current_weather_data.windScale);
        lcd_show_string(0, 100, 150, 16, 16, "Wind:", BLUE);
        char wind_copy[8];
        strncpy(wind_copy, wind_dir_en, sizeof(wind_copy) - 1);
        wind_copy[sizeof(wind_copy) - 1] = '\0';
        lcd_show_string(40, 100, 150, 16, 16, wind_copy, RED);
        lcd_show_string(0, 120, 150, 16, 16, "Spd:", BLUE);
        lcd_show_string(40, 120, 150, 16, 16, s_current_weather_data.windScale, RED);
        
        /* 湿度 */
        char humidity_str[16];
        snprintf(humidity_str, sizeof(humidity_str), "%s%%", s_current_weather_data.humidity);
        ESP_LOGV(TAG, "Humidity: %s", humidity_str);
        lcd_show_string(0, 140, 150, 16, 16, "Hum:", BLUE);
        lcd_show_string(40, 140, 150, 16, 16, humidity_str, RED);
    } else {
        ESP_LOGV(TAG, "No weather data available");
        lcd_show_string(0, 40, 150, 16, 16, "No Weather Data", RED);
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
    
    /* 清屏 */
    lcd_fill(0, 0, 79, 159, WHITE);
    
    /* 显示消息（需要转换为非const指针） */
    char msg_copy[128];
    strncpy(msg_copy, message, sizeof(msg_copy) - 1);
    msg_copy[sizeof(msg_copy) - 1] = '\0';
    lcd_show_string(0, 0, 150, 16, 16, msg_copy, color);
    
    return ESP_OK;
}
