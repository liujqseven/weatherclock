/**
 * @file display_manager.h
 * @brief 显示管理模块
 * @author ESP-IDF Version
 */

#ifndef __DISPLAY_MANAGER_H
#define __DISPLAY_MANAGER_H

#include <stdint.h>
#include "esp_err.h"
#include "weather_client.h"

/* 函数声明 */
esp_err_t display_manager_init(void);
esp_err_t display_manager_update(void);
esp_err_t display_manager_update_time(void);
esp_err_t display_manager_update_weather(const weather_data_t *weather_data);
esp_err_t display_manager_show_message(const char *message, uint16_t color);
esp_err_t display_manager_clear_messages(void);

#endif /* __DISPLAY_MANAGER_H */
