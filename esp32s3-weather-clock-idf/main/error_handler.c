/**
 * @file error_handler.c
 * @brief 错误处理模块实现
 * @author ESP-IDF Version
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "error_handler.h"

static const char *TAG = "ERROR_HANDLER";

/**
 * @brief 错误码到错误消息的映射
 */
static const struct {
    error_code_t code;
    const char *message;
} error_messages[] = {
    /* 系统错误 */
    { ERR_SYSTEM_INIT_FAILED, "System initialization failed" },
    { ERR_TASK_CREATE_FAILED, "Task creation failed" },
    { ERR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed" },
    
    /* WiFi错误 */
    { ERR_WIFI_CONNECT_FAILED, "WiFi connection failed" },
    { ERR_WIFI_DISCONNECTED, "WiFi disconnected" },
    { ERR_WIFI_SCAN_FAILED, "WiFi scan failed" },
    
    /* NTP错误 */
    { ERR_NTP_SYNC_FAILED, "NTP synchronization failed" },
    { ERR_NTP_SERVER_UNREACHABLE, "NTP server unreachable" },
    
    /* 天气API错误 */
    { ERR_WEATHER_FETCH_FAILED, "Weather data fetch failed" },
    { ERR_WEATHER_PARSE_FAILED, "Weather data parse failed" },
    { ERR_WEATHER_API_ERROR, "Weather API error" },
    { ERR_WEATHER_BUFFER_OVERFLOW, "Weather buffer overflow" },
    
    /* 显示错误 */
    { ERR_DISPLAY_INIT_FAILED, "Display initialization failed" },
    { ERR_DISPLAY_UPDATE_FAILED, "Display update failed" },
    
    /* 其他错误 */
    { ERR_UNKNOWN, "Unknown error" },
    { 0, NULL }
};

/**
 * @brief 初始化错误处理器
 */
esp_err_t error_handler_init(void)
{
    ESP_LOGI(TAG, "Initializing error handler");
    return ESP_OK;
}

/**
 * @brief 记录错误
 */
esp_err_t error_handler_log(error_code_t code, const char *message, const char *file, int line)
{
    const char *error_msg = error_handler_get_message(code);
    
    ESP_LOGE(TAG, "[%s:%d] Error %d: %s - %s", 
             file, line, code, error_msg, message ? message : "");
    
    return ESP_OK;
}

/**
 * @brief 获取错误信息
 */
const char *error_handler_get_message(error_code_t code)
{
    for (int i = 0; error_messages[i].message != NULL; i++) {
        if (error_messages[i].code == code) {
            return error_messages[i].message;
        }
    }
    return "Unknown error";
}

/**
 * @brief 检查错误并记录
 */
bool error_handler_check(esp_err_t err, error_code_t custom_code, const char *message, const char *file, int line)
{
    if (err != ESP_OK) {
        const char *esp_err_msg = esp_err_to_name(err);
        const char *custom_err_msg = error_handler_get_message(custom_code);
        
        ESP_LOGE(TAG, "[%s:%d] ESP error: %s - Custom error: %d %s - %s", 
                 file, line, esp_err_msg, custom_code, custom_err_msg, message ? message : "");
        
        return true;
    }
    
    return false;
}
