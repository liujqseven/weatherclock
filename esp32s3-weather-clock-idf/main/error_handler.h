/**
 * @file error_handler.h
 * @brief 错误处理模块头文件
 * @author ESP-IDF Version
 */

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief 自定义错误码枚举
 */
typedef enum {
    /* 系统错误 */
    ERR_SYSTEM_INIT_FAILED = 0x1000,
    ERR_TASK_CREATE_FAILED,
    ERR_MEMORY_ALLOCATION_FAILED,
    
    /* WiFi错误 */
    ERR_WIFI_CONNECT_FAILED = 0x2000,
    ERR_WIFI_DISCONNECTED,
    ERR_WIFI_SCAN_FAILED,
    
    /* NTP错误 */
    ERR_NTP_SYNC_FAILED = 0x3000,
    ERR_NTP_SERVER_UNREACHABLE,
    
    /* 天气API错误 */
    ERR_WEATHER_FETCH_FAILED = 0x4000,
    ERR_WEATHER_PARSE_FAILED,
    ERR_WEATHER_API_ERROR,
    ERR_WEATHER_BUFFER_OVERFLOW,
    
    /* 显示错误 */
    ERR_DISPLAY_INIT_FAILED = 0x5000,
    ERR_DISPLAY_UPDATE_FAILED,
    
    /* 其他错误 */
    ERR_UNKNOWN = 0xFFFF
} error_code_t;

/**
 * @brief 错误信息结构体
 */
typedef struct {
    error_code_t code;
    const char *message;
    const char *file;
    int line;
} error_info_t;

/**
 * @brief 初始化错误处理器
 * @return ESP_OK 成功
 * @return 其他错误码 失败
 */
esp_err_t error_handler_init(void);

/**
 * @brief 记录错误
 * @param code 错误码
 * @param message 错误消息
 * @param file 文件名
 * @param line 行号
 * @return ESP_OK 成功
 * @return 其他错误码 失败
 */
esp_err_t error_handler_log(error_code_t code, const char *message, const char *file, int line);

/**
 * @brief 获取错误信息
 * @param code 错误码
 * @return 错误信息字符串
 */
const char *error_handler_get_message(error_code_t code);

/**
 * @brief 检查错误并记录
 * @param err ESP错误码
 * @param custom_code 自定义错误码
 * @param message 错误消息
 * @param file 文件名
 * @param line 行号
 * @return true 有错误
 * @return false 无错误
 */
bool error_handler_check(esp_err_t err, error_code_t custom_code, const char *message, const char *file, int line);

#endif /* ERROR_HANDLER_H */
