/**
 * @file ntp_client.h
 * @brief NTP时间同步客户端头文件
 * @author ESP-IDF Version
 */

#ifndef __NTP_CLIENT_H
#define __NTP_CLIENT_H

#include <stdbool.h>
#include <time.h>
#include "esp_err.h"

/* NTP服务器配置 */
#define NTP_SERVER1         "pool.ntp.org"
#define NTP_SERVER2         "time.nist.gov"
#define TIMEZONE_OFFSET     8 * 3600  // 东八区时区（秒）
#define NTP_SYNC_TIMEOUT    10000     // NTP同步超时时间（毫秒）
#define NTP_MAX_RETRY       3         // 最大重试次数

/* 手动时间备用（NTP同步失败时使用） */
#define MANUAL_START_HOUR   12
#define MANUAL_START_MINUTE 0
#define MANUAL_START_SECOND 0
#define MANUAL_START_YEAR   2009
#define MANUAL_START_MONTH  1
#define MANUAL_START_DAY    1

/* 函数声明 */
esp_err_t ntp_client_init(void);
bool ntp_client_is_synced(void);
bool ntp_client_get_time(struct tm *timeinfo);
void ntp_client_update_manual_time(void);

#endif
