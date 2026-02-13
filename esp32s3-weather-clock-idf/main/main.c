/**
 * @file main.c
 * @brief 天气时钟主应用程序
 * @author ESP-IDF Version
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_system.h"

#include "led.h"
#include "wifi_manager.h"
#include "ntp_client.h"
#include "weather_client.h"
#include "lcd.h"

/* 使用LCD头文件中定义的颜色宏 */
// 颜色定义已在 lcd.h 中定义，直接使用即可

static const char *TAG = "MAIN_APP";

/* 任务句柄 */
static TaskHandle_t weather_update_task_handle = NULL;
static TaskHandle_t display_update_task_handle = NULL;
static TaskHandle_t led_blink_task_handle = NULL;

/* 全局数据 */
static weather_data_t current_weather_data = {0};
static bool weather_data_available = false;

/**
 * @brief 将中文天气状况转换为英文
 */
static char* translate_weather(const char* condition)
{
    if (condition == NULL) return "N/A";
    
    if (strcmp(condition, "晴") == 0) return "SUNNY";
    if (strcmp(condition, "多云") == 0) return "CLOUDY";
    if (strcmp(condition, "阴") == 0) return "OVERCAST";
    if (strcmp(condition, "雨") == 0) return "RAIN";
    if (strcmp(condition, "雪") == 0) return "SNOW";
    if (strcmp(condition, "雾") == 0) return "FOG";
    
    return (char*)condition; /* 如果没有匹配的，返回原始值 */
}

/**
 * @brief LED闪烁任务
 */
static void led_blink_task(void *pvParameters)
{
    ESP_LOGI(TAG, "LED blink task started");
    
    while (1) {
        LED_TOGGLE();
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1秒闪烁一次
    }
}

/**
 * @brief 天气更新任务
 */
static void weather_update_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Weather update task started");
    
    while (1) {
        /* 检查WiFi连接状态 */
        if (wifi_manager_is_connected()) {
            /* 获取天气数据 */
            esp_err_t err = weather_client_fetch_data(&current_weather_data);
            if (err == ESP_OK) {
                weather_data_available = true;
                ESP_LOGI(TAG, "Weather data updated successfully");
                
                /* 天气数据更新后立即刷新屏幕 */
                if (display_update_task_handle) {
                    xTaskNotifyGive(display_update_task_handle);
                }
            } else {
                ESP_LOGE(TAG, "Failed to update weather data: %s", esp_err_to_name(err));
            }
        } else {
            ESP_LOGW(TAG, "WiFi not connected, skipping weather update");
        }
        
        /* 每30分钟更新一次天气数据 */
        vTaskDelay(pdMS_TO_TICKS(30 * 60 * 1000));
    }
}

/**
 * @brief 显示更新任务
 */
static void display_update_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Display update task started");
    
    while (1) {
        /* 等待通知或超时 */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(30000)); // 30秒超时
        
        /* 每次都全屏刷新，确保显示正确 */
        lcd_fill(0, 0, 159, 79, WHITE);
        
        /* 显示时间信息 */
        struct tm timeinfo;
        if (ntp_client_get_time(&timeinfo)) {
            char time_str[32];
            strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);
            lcd_show_string(5, 5, 150, 16, 16, time_str, BLACK);
            
            char date_str[32];
            strftime(date_str, sizeof(date_str), "%Y-%m-%d", &timeinfo);
            lcd_show_string(5, 25, 150, 16, 16, date_str, BLACK);
        } else {
            lcd_show_string(5, 5, 150, 16, 16, "--:--:--", BLACK);
        }
        
        if (weather_data_available) {
            /* 显示天气数据 */
            lcd_show_string(5, 45, 150, 16, 16, "Temp:", RED);
            lcd_show_string(60, 45, 150, 16, 16, current_weather_data.temp, RED);
            lcd_show_string(100, 45, 150, 16, 16, "WX:", BLUE);
            
            /* 转换天气状况为英文 */
            char* weather_en = translate_weather(current_weather_data.text);
            lcd_show_string(130, 45, 150, 16, 16, weather_en, BLUE);
        } else {
            /* 显示无数据信息 */
            lcd_show_string(5, 45, 150, 16, 16, "No Weather Data", RED);
        }
    }
}

/**
 * @brief 应用程序主入口
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32-S3M 天气显示系统启动 ===");
    
    /* 初始化LED */
    led_init();
    LED_ON();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    /* 初始化SPI和LCD */
    spi2_init();
    lcd_init();
    lcd_fill(0, 0, 159, 79, WHITE);
    lcd_show_string(5, 5, 150, 16, 16, "Weather Clock", BLACK);
    lcd_show_string(5, 25, 150, 16, 16, "Starting...", BLACK);
    
    /* 初始化WiFi */
    lcd_show_string(5, 45, 150, 16, 16, "WiFi Connecting", BLACK);
    esp_err_t err = wifi_manager_init();
    if (err == ESP_OK) {
        lcd_show_string(5, 45, 150, 16, 16, "WiFi Connected!", BLACK);
        ESP_LOGI(TAG, "WiFi initialized successfully");
    } else {
        lcd_show_string(5, 45, 150, 16, 16, "WiFi Failed!", RED);
        ESP_LOGE(TAG, "WiFi initialization failed: %s", esp_err_to_name(err));
    }
    
    /* 初始化NTP客户端 */
    lcd_show_string(5, 62, 150, 16, 16, "Sync Time...", BLACK);
    err = ntp_client_init();
    if (err == ESP_OK) {
        lcd_show_string(5, 62, 150, 16, 16, "Time Synced!", BLACK);
        ESP_LOGI(TAG, "NTP client initialized successfully");
    } else {
        lcd_show_string(5, 62, 150, 16, 16, "Use Manual Time", BLUE);
        ESP_LOGW(TAG, "NTP client initialization failed, using manual time fallback");
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* 初始化天气客户端 */
    weather_client_init();
    
    /* 获取初始天气数据 */
    lcd_show_string(5, 62, 150, 16, 16, "Fetching WX...", BLACK);
    if (wifi_manager_is_connected()) {
        err = weather_client_fetch_data(&current_weather_data);
        if (err == ESP_OK) {
            weather_data_available = true;
            lcd_show_string(5, 62, 150, 16, 16, "WX Data OK!", BLACK);
            ESP_LOGI(TAG, "Initial weather data fetched successfully");
        } else {
            lcd_show_string(5, 62, 150, 16, 16, "WX Data Fail!", RED);
            ESP_LOGE(TAG, "Failed to fetch initial weather data");
        }
    } else {
        lcd_show_string(5, 62, 150, 16, 16, "No WiFi!", RED);
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* 清除启动信息区域 */
    lcd_fill(0, 0, 159, 79, WHITE);
    
    /* 创建LED闪烁任务（500ms周期） */
    BaseType_t led_task_result = xTaskCreate(led_blink_task, "led_blink", 2048, 
                                           NULL, 3, &led_blink_task_handle);
    if (led_task_result == pdPASS) {
        ESP_LOGI(TAG, "LED blink task created successfully");
    } else {
        ESP_LOGE(TAG, "Failed to create LED blink task: %d", led_task_result);
    }
    
    /* 创建天气更新任务（需要较大的栈空间用于TLS和HTTP操作） */
    xTaskCreate(weather_update_task, "weather_update", 16384, 
                NULL, 5, &weather_update_task_handle);
    
    /* 创建显示更新任务 */
    xTaskCreate(display_update_task, "display_update", 4096, 
                NULL, 4, &display_update_task_handle);
    
    ESP_LOGI(TAG, "=== 系统初始化完成 ===");
    
    /* 主任务可以执行其他后台操作 */
    while (1) {
        /* 检查系统健康状态 */
        ESP_LOGD(TAG, "System running...");
        ESP_LOGD(TAG, "  Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
        ESP_LOGD(TAG, "  Minimum free heap: %lu bytes", (unsigned long)esp_get_minimum_free_heap_size());
        
        /* 检查任务栈使用情况 */
        if (weather_update_task_handle) {
            UBaseType_t stack_high_water_mark = uxTaskGetStackHighWaterMark(weather_update_task_handle);
            ESP_LOGD(TAG, "  Weather task stack high water mark: %lu bytes", (unsigned long)stack_high_water_mark);
        }
        
        if (display_update_task_handle) {
            UBaseType_t stack_high_water_mark = uxTaskGetStackHighWaterMark(display_update_task_handle);
            ESP_LOGD(TAG, "  Display task stack high water mark: %lu bytes", (unsigned long)stack_high_water_mark);
        }
        
        /* 每10秒检查一次 */
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
