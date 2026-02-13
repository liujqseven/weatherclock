/**
 * @file system_init.h
 * @brief 系统初始化模块头文件
 * @author ESP-IDF Version
 */

#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief 系统模块初始化状态
 */
typedef struct {
    bool led_initialized;
    bool display_initialized;
    bool wifi_initialized;
    bool ntp_initialized;
    bool weather_initialized;
} system_init_status_t;

/**
 * @brief 初始化系统所有模块
 * @return ESP_OK 成功
 * @return 其他错误码 失败
 */
esp_err_t system_init(void);

/**
 * @brief 获取系统初始化状态
 * @param status 状态结构体指针
 * @return ESP_OK 成功
 * @return 其他错误码 失败
 */
esp_err_t system_get_init_status(system_init_status_t *status);

/**
 * @brief 清理系统所有模块
 * @return ESP_OK 成功
 * @return 其他错误码 失败
 */
esp_err_t system_cleanup(void);

#endif /* SYSTEM_INIT_H */
