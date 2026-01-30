/**
 * @file weather_client.h
 * @brief 天气API客户端头文件
 * @author ESP-IDF Version
 */

#ifndef __WEATHER_CLIENT_H
#define __WEATHER_CLIENT_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* 和风天气API配置 */
#define API_HOST            "devapi.qweatherapi.com"
#define API_KEY             "api_key_token"
#define LOCATION_ID         "101020100"  // 上海市ID

/* 天气数据结构体 */
typedef struct {
    char temp[8];              // 温度
    char feelsLike[8];         // 体感温度
    char text[32];             // 天气状况
    char windDir[16];          // 风向
    char windScale[8];         // 风力等级
    char humidity[8];          // 湿度
    char vis[8];               // 能见度
    char updateTime[32];       // 更新时间
} weather_data_t;

/* 函数声明 */
esp_err_t weather_client_init(void);
esp_err_t weather_client_fetch_data(weather_data_t *data);
bool weather_client_has_valid_data(void);

#endif
