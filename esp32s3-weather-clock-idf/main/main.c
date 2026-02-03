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
#include "lcd_display.h"

static const char *TAG = "MAIN_APP";

/* 任务句柄 */
static TaskHandle_t weather_update_task_handle = NULL;
static TaskHandle_t display_update_task_handle = NULL;
static TaskHandle_t led_blink_task_handle = NULL;

/* 全局数据 */
static weather_data_t current_weather_data = {0};
static bool weather_data_available = false;

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
        /* 检查是否有有效的天气数据 */
        if (weather_data_available) {
            lcd_display_weather_data(&current_weather_data);
        } else {
            lcd_display_error("No Data");
        }
        
        /* 每秒更新一次显示 */
        vTaskDelay(pdMS_TO_TICKS(1000));
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
    
    /* 初始化LCD显示 */
    lcd_display_init();
    lcd_display_boot_message();
    
    /* 初始化WiFi */
    lcd_display_wifi_status(25, "WiFi Connecting");
    esp_err_t err = wifi_manager_init();
    if (err == ESP_OK) {
        lcd_display_wifi_status(35, "WiFi Connected!");
        ESP_LOGI(TAG, "WiFi initialized successfully");
    } else {
        lcd_display_wifi_status(35, "WiFi Failed!");
        ESP_LOGE(TAG, "WiFi initialization failed");
    }
    
    /* 初始化NTP客户端 */
    lcd_display_wifi_status(55, "Sync Time...");
    err = ntp_client_init();
    if (err == ESP_OK) {
        lcd_display_wifi_status(55, "Time Synced!");
        ESP_LOGI(TAG, "NTP client initialized successfully");
    } else {
        lcd_display_wifi_status(55, "Use Manual Time");
        ESP_LOGW(TAG, "NTP client initialization failed, using manual time fallback");
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* 初始化天气客户端 */
    weather_client_init();
    
    /* 获取初始天气数据 */
    lcd_display_wifi_status(45, "Fetching WX...");
    if (wifi_manager_is_connected()) {
        err = weather_client_fetch_data(&current_weather_data);
        if (err == ESP_OK) {
            weather_data_available = true;
            lcd_display_wifi_status(45, "WX Data OK!");
            ESP_LOGI(TAG, "Initial weather data fetched successfully");
        } else {
            lcd_display_wifi_status(45, "WX Data Fail!");
            ESP_LOGE(TAG, "Failed to fetch initial weather data");
        }
    } else {
        lcd_display_wifi_status(45, "No WiFi!");
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* 清除启动信息区域 */
    lcd_display_fill_rect(0, 0, SCREEN_WIDTH, 23, COLOR_BLACK);
    
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
