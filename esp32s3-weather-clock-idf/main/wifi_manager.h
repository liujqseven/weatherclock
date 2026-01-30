/**
 * @file wifi_manager.h
 * @brief WiFi连接管理头文件
 * @author ESP-IDF Version
 */

#ifndef __WIFI_MANAGER_H
#define __WIFI_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"

/* WiFi连接设置 */
#define WIFI_SSID           "WIFI_SSID"
#define WIFI_PASSWORD       "WIFI_PASSWORD"
#define WIFI_CONNECT_TIMEOUT 20      // 连接超时时间（秒）
#define WIFI_RETRY_INTERVAL 500     // 重试间隔（毫秒）

/* WiFi连接状态 */
typedef enum {
    WIFI_STATUS_DISCONNECTED = 0,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_FAILED
} wifi_status_t;

/* 回调函数类型 */
typedef void (*wifi_status_callback_t)(wifi_status_t status);

/* 函数声明 */
esp_err_t wifi_manager_init(void);
wifi_status_t wifi_manager_get_status(void);
void wifi_manager_register_callback(wifi_status_callback_t callback);
bool wifi_manager_is_connected(void);

#endif
