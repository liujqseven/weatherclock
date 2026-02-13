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
    
    ESP_LOGI(TAG, "Display manager initialized successfully");
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
        lcd_show_string(0, 40, 150, 16, 16, "Temp:", RED);
        lcd_show_string(40, 40, 150, 16, 16, temp_str, RED);
        
        /* 天气状况 */
        const char* weather_en = translate_weather(s_current_weather_data.text);
        ESP_LOGV(TAG, "Weather condition: %s", weather_en);
        lcd_show_string(0, 60, 150, 16, 16, "WX:", BLUE);
        char weather_copy[16];
        strncpy(weather_copy, weather_en, sizeof(weather_copy) - 1);
        weather_copy[sizeof(weather_copy) - 1] = '\0';
        lcd_show_string(40, 60, 150, 16, 16, weather_copy, BLUE);
        
        /* 体感温度 */
        char feels_like_str[16];
        snprintf(feels_like_str, sizeof(feels_like_str), "%sC", s_current_weather_data.feelsLike);
        ESP_LOGV(TAG, "Feels like: %s", feels_like_str);
        lcd_show_string(0, 80, 150, 16, 16, "Feel:", BLUE);
        lcd_show_string(40, 80, 150, 16, 16, feels_like_str, BLUE);
        
        /* 风向和风力 */
        const char* wind_dir_en = translate_wind_dir(s_current_weather_data.windDir);
        ESP_LOGV(TAG, "Wind direction: %s, scale: %s", wind_dir_en, s_current_weather_data.windScale);
        lcd_show_string(0, 100, 150, 16, 16, "Wind:", BLACK);
        char wind_copy[8];
        strncpy(wind_copy, wind_dir_en, sizeof(wind_copy) - 1);
        wind_copy[sizeof(wind_copy) - 1] = '\0';
        lcd_show_string(40, 100, 150, 16, 16, wind_copy, BLACK);
        lcd_show_string(0, 120, 150, 16, 16, "Spd:", BLACK);
        lcd_show_string(40, 120, 150, 16, 16, s_current_weather_data.windScale, BLACK);
        
        /* 湿度 */
        char humidity_str[16];
        snprintf(humidity_str, sizeof(humidity_str), "%s%%", s_current_weather_data.humidity);
        ESP_LOGV(TAG, "Humidity: %s", humidity_str);
        lcd_show_string(0, 140, 150, 16, 16, "Hum:", RED);
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
