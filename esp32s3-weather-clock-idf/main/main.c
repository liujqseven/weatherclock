/**
 * @file main.c
 * @brief 天气时钟主应用程序
 * @author ESP-IDF Version
 * 
 * 主应用程序文件，负责：
 * 1. 初始化系统所有模块
 * 2. 创建和管理系统任务
 * 3. 监控系统运行状态
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_system.h"

#include "led.h"
#include "system_init.h"
#include "task_manager.h"
#include "wifi_manager.h"
#include "weather_client.h"
#include "display_manager.h"

static const char *TAG = "MAIN_APP";

/**
 * @brief LED闪烁任务
 * 
 * 任务功能：控制LED灯每5秒闪烁一次
 * @param pvParameters 任务参数（未使用）
 */
static void led_blink_task(void *pvParameters)
{
    ESP_LOGI(TAG, "LED blink task started");
    
    while (1) {
        LED_TOGGLE();
        vTaskDelay(pdMS_TO_TICKS(5000));  // 5秒闪烁一次
    }
}

/**
 * @brief 天气更新任务
 * 
 * 任务功能：
 * 1. 检查WiFi连接状态
 * 2. 每30分钟获取一次天气数据
 * 3. 更新天气数据后刷新屏幕显示
 * @param pvParameters 任务参数（未使用）
 */
static void weather_update_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Weather update task started");
    
    /* 任务启动后先延迟30分钟再获取天气数据，因为系统初始化时已经获取了一次 */
    ESP_LOGI(TAG, "First weather update in 30 minutes (system init already fetched initial data)");
    vTaskDelay(pdMS_TO_TICKS(30 * 60 * 1000));
    
    int update_count = 0;
    
    while (1) {
        update_count++;
        ESP_LOGI(TAG, "Weather update cycle %d starting", update_count);
        
        /* 检查WiFi连接状态 */
        bool is_connected = wifi_manager_is_connected();
        ESP_LOGI(TAG, "WiFi connection status: %s", is_connected ? "CONNECTED" : "DISCONNECTED");
        
        if (is_connected) {
            /* 获取天气数据 */
            weather_data_t weather_data;
            ESP_LOGI(TAG, "Fetching weather data...");
            esp_err_t err = weather_client_fetch_data(&weather_data);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Weather data updated successfully");
                
                /* 天气数据更新后立即刷新屏幕 */
                display_manager_update_weather(&weather_data);
                ESP_LOGI(TAG, "Weather display updated");
            } else {
                ESP_LOGE(TAG, "Failed to update weather data: %s", esp_err_to_name(err));
            }
        } else {
            ESP_LOGW(TAG, "WiFi not connected, skipping weather update");
        }
        
        /* 每30分钟更新一次天气数据 */
        ESP_LOGI(TAG, "Weather update cycle %d completed, next update in 30 minutes", update_count);
        vTaskDelay(pdMS_TO_TICKS(30 * 60 * 1000));
    }
}

/**
 * @brief 显示更新任务
 * 
 * 任务功能：
 * 1. 首次运行时显示完整时间
 * 2. 每秒只更新变化的时间部分
 * 3. 不刷新整个屏幕，只更新时间区域
 * @param pvParameters 任务参数（未使用）
 */
static void display_update_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Display update task started");
    
    /* 等待一段时间，让其他任务先启动 */
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    /* 首次运行时显示完整时间 */
    display_manager_update();
    
    while (1) {
        /* 只更新时间显示 */
        display_manager_update_time();
        
        /* 每秒更新一次 */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief 系统健康检查任务
 * 
 * 任务功能：
 * 1. 每10秒检查一次系统健康状态
 * 2. 打印系统堆内存使用情况
 * 3. 打印各任务栈使用情况
 * @param pvParameters 任务参数（未使用）
 */
static void system_health_check_task(void *pvParameters)
{
    ESP_LOGI(TAG, "System health check task started");
    
    /* 等待一段时间，让其他任务先启动 */
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    while (1) {
        /* 检查系统健康状态 */
        ESP_LOGD(TAG, "System running...");
        ESP_LOGD(TAG, "  Free heap: %" PRIu32 " bytes", esp_get_free_heap_size());
        ESP_LOGD(TAG, "  Minimum free heap: %" PRIu32 " bytes", esp_get_minimum_free_heap_size());
        
        /* 检查任务栈使用情况 */
        for (int i = 0; i < TASK_TYPE_MAX; i++) {
            task_info_t task_info;
            if (task_manager_get_task_info((task_type_t)i, &task_info) == ESP_OK && task_info.is_running) {
                if (task_info.task_handle != NULL) {
                    UBaseType_t stack_high_water_mark = uxTaskGetStackHighWaterMark(task_info.task_handle);
                    ESP_LOGD(TAG, "  %s task stack high water mark: %u bytes", task_info.task_name, stack_high_water_mark);
                }
            }
        }
        
        /* 每10秒检查一次 */
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/**
 * @brief 创建系统任务
 * 
 * 函数功能：
 * 1. 初始化任务管理器
 * 2. 创建LED闪烁任务
 * 3. 创建天气更新任务
 * 4. 创建系统健康检查任务
 * @return ESP_OK 成功
 * @return 其他错误码 失败
 */
static esp_err_t create_system_tasks(void)
{
    ESP_LOGD(TAG, "创建系统任务");
    
    /* 初始化任务管理器 */
    esp_err_t err = task_manager_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Task manager initialization failed: %s", esp_err_to_name(err));
        return err;
    }
    
    /* 创建LED闪烁任务 */
    err = task_manager_create_task(TASK_TYPE_LED_BLINK, led_blink_task, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LED blink task: %s", esp_err_to_name(err));
        return err;
    }
    
    /* 创建天气更新任务 */
    err = task_manager_create_task(TASK_TYPE_WEATHER_UPDATE, weather_update_task, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create weather update task: %s", esp_err_to_name(err));
        return err;
    }
    
    /* 创建显示更新任务 */
    err = task_manager_create_task(TASK_TYPE_DISPLAY_UPDATE, display_update_task, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create display update task: %s", esp_err_to_name(err));
        return err;
    }
    
    /* 创建系统健康检查任务 */
    err = task_manager_create_task(TASK_TYPE_SYSTEM_HEALTH, system_health_check_task, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create system health check task: %s", esp_err_to_name(err));
        return err;
    }
    
    return ESP_OK;
}

/**
 * @brief 应用程序主入口
 * 
 * 函数功能：
 * 1. 初始化系统所有模块
 * 2. 创建系统任务
 * 3. 监控系统运行状态
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32-S3 天气时钟系统启动 ===");
    
    /* 初始化系统所有模块 */
    esp_err_t err = system_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "System initialization failed: %s", esp_err_to_name(err));
        return;
    }
    
    /* 创建系统任务 */
    err = create_system_tasks();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create system tasks: %s", esp_err_to_name(err));
        return;
    }
    
    ESP_LOGI(TAG, "=== 系统初始化完成 ===");
}

