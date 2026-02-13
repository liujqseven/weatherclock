/**
 * @file system_init.c
 * @brief 系统初始化模块实现
 * @author ESP-IDF Version
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led.h"
#include "lcd.h"
#include "display_manager.h"
#include "wifi_manager.h"
#include "ntp_client.h"
#include "weather_client.h"
#include "error_handler.h"
#include "system_init.h"

#define ERROR_CHECK(err, code, msg) error_handler_check((err), (code), (msg), __FILE__, __LINE__)

static const char *TAG = "SYSTEM_INIT";

/* 系统初始化状态 */
static system_init_status_t s_init_status = {0};
static bool s_system_initialized = false;

/**
 * @brief 初始化系统所有模块
 */
esp_err_t system_init(void)
{
    /* 防止重复初始化 */
    if (s_system_initialized) {
        ESP_LOGI(TAG, "System already initialized, skipping...");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "=== 开始系统初始化 ===");
    
    /* 初始化LED */
    ESP_LOGD(TAG, "初始化LED模块");
    led_init();
    LED_ON();
    vTaskDelay(pdMS_TO_TICKS(2000));
    s_init_status.led_initialized = true;
    ESP_LOGI(TAG, "LED模块初始化完成");
    
    /* 初始化错误处理器 */
    error_handler_init();
    
    /* 初始化显示管理器 */
    ESP_LOGD(TAG, "初始化显示管理器");
    esp_err_t err = display_manager_init();
    if (ERROR_CHECK(err, ERR_DISPLAY_INIT_FAILED, "Display manager initialization failed")) {
        return err;
    }
    s_init_status.display_initialized = true;
    ESP_LOGI(TAG, "显示管理器初始化完成");
    
    /* 显示启动信息 */
    display_manager_show_message("Starting WiFi...", BLACK);
    
    /* 初始化WiFi */
    ESP_LOGD(TAG, "初始化WiFi模块");
    err = wifi_manager_init();
    if (ERROR_CHECK(err, ERR_WIFI_CONNECT_FAILED, "WiFi initialization failed")) {
        display_manager_show_message("WiFi Failed!", RED);
        /* WiFi失败不阻止其他模块初始化 */
    } else {
        display_manager_show_message("WiFi Connected!", BLACK);
        s_init_status.wifi_initialized = true;
        ESP_LOGI(TAG, "WiFi模块初始化完成");
    }
    
    /* 显示时间同步信息 */
    display_manager_show_message("Syncing Time...", BLACK);
    
    /* 初始化NTP客户端 */
    ESP_LOGD(TAG, "初始化NTP客户端");
    err = ntp_client_init();
    if (ERROR_CHECK(err, ERR_NTP_SYNC_FAILED, "NTP client initialization failed")) {
        display_manager_show_message("Use Manual Time", BLUE);
        /* NTP失败不阻止其他模块初始化 */
    } else {
        display_manager_show_message("Time Synced!", BLACK);
        s_init_status.ntp_initialized = true;
        ESP_LOGI(TAG, "NTP客户端初始化完成");
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* 初始化天气客户端 */
    ESP_LOGD(TAG, "初始化天气客户端");
    weather_client_init();
    s_init_status.weather_initialized = true;
    ESP_LOGI(TAG, "天气客户端初始化完成");
    
    /* 显示天气数据获取信息 */
    display_manager_show_message("Fetching WX...", BLACK);
    
    /* 获取初始天气数据 */
    if (s_init_status.wifi_initialized && wifi_manager_is_connected()) {
        weather_data_t weather_data;
        err = weather_client_fetch_data(&weather_data);
        if (ERROR_CHECK(err, ERR_WEATHER_FETCH_FAILED, "Initial weather data fetch failed")) {
            display_manager_show_message("WX Data Fail!", RED);
        } else {
            display_manager_show_message("WX Data OK!", BLACK);
            display_manager_update_weather(&weather_data);
            ESP_LOGI(TAG, "初始天气数据获取成功");
        }
    } else {
        display_manager_show_message("No WiFi!", RED);
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* 标记系统初始化完成 */
    s_system_initialized = true;
    
    ESP_LOGI(TAG, "=== 系统初始化完成 ===");
    return ESP_OK;
}

/**
 * @brief 获取系统初始化状态
 */
esp_err_t system_get_init_status(system_init_status_t *status)
{
    if (status == NULL) {
        ESP_LOGE(TAG, "无效的状态指针");
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(status, &s_init_status, sizeof(system_init_status_t));
    return ESP_OK;
}

/**
 * @brief 清理系统所有模块
 */
esp_err_t system_cleanup(void)
{
    ESP_LOGI(TAG, "=== 开始系统清理 ===");
    
    /* 清理顺序：先清理依赖其他模块的模块 */
    
    /* 清理天气客户端 */
    if (s_init_status.weather_initialized) {
        ESP_LOGD(TAG, "清理天气客户端");
        /* 天气客户端目前没有清理函数，预留 */
        s_init_status.weather_initialized = false;
    }
    
    /* 清理NTP客户端 */
    if (s_init_status.ntp_initialized) {
        ESP_LOGD(TAG, "清理NTP客户端");
        /* NTP客户端目前没有清理函数，预留 */
        s_init_status.ntp_initialized = false;
    }
    
    /* 清理WiFi */
    if (s_init_status.wifi_initialized) {
        ESP_LOGD(TAG, "清理WiFi模块");
        /* WiFi模块目前没有清理函数，预留 */
        s_init_status.wifi_initialized = false;
    }
    
    /* 清理显示管理器 */
    if (s_init_status.display_initialized) {
        ESP_LOGD(TAG, "清理显示管理器");
        /* 显示管理器目前没有清理函数，预留 */
        s_init_status.display_initialized = false;
    }
    
    /* 清理LED */
    if (s_init_status.led_initialized) {
        ESP_LOGD(TAG, "清理LED模块");
        LED_OFF();
        s_init_status.led_initialized = false;
    }
    
    /* 重置初始化状态 */
    s_system_initialized = false;
    
    ESP_LOGI(TAG, "=== 系统清理完成 ===");
    return ESP_OK;
}
