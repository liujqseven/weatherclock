/**
 * @file ntp_client.c
 * @brief NTP时间同步客户端实现文件
 * @author ESP-IDF Version
 */

#include "ntp_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sntp.h"

static const char *TAG = "NTP_CLIENT";

/* 全局变量 */
static bool s_ntp_synced = false;
static time_t s_manual_start_time = 0;

/* SNTP同步完成回调 */
static void sntp_time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "NTP time synchronized!");
    s_ntp_synced = true;
    
    struct tm timeinfo;
    if (localtime_r(&tv->tv_sec, &timeinfo) != NULL) {
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
        ESP_LOGI(TAG, "Current time: %s", time_str);
    }
}

/**
 * @brief 初始化NTP客户端
 */
esp_err_t ntp_client_init(void)
{
    ESP_LOGI(TAG, "Initializing NTP client...");
    
    /* 配置时区 */
    setenv("TZ", "CST-8", 1);
    tzset();
    
    /* 配置SNTP */
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER1);
    esp_sntp_setservername(1, NTP_SERVER2);
    esp_sntp_set_time_sync_notification_cb(sntp_time_sync_notification_cb);
    
    /* 初始化SNTP */
    esp_sntp_init();
    
    /* 等待时间同步 */
    int retry = 0;
    while (retry < NTP_MAX_RETRY && !s_ntp_synced) {
        ESP_LOGI(TAG, "Waiting for NTP sync... (%d/%d)", retry + 1, NTP_MAX_RETRY);
        vTaskDelay(pdMS_TO_TICKS(NTP_SYNC_TIMEOUT / NTP_MAX_RETRY));
        retry++;
    }
    
    /* 如果NTP同步失败，初始化手动时间 */
    if (!s_ntp_synced) {
        ESP_LOGW(TAG, "NTP sync failed, using manual time fallback");
        struct tm manual_time = {
            .tm_year = MANUAL_START_YEAR - 1900,
            .tm_mon = MANUAL_START_MONTH - 1,
            .tm_mday = MANUAL_START_DAY,
            .tm_hour = MANUAL_START_HOUR,
            .tm_min = MANUAL_START_MINUTE,
            .tm_sec = MANUAL_START_SECOND
        };
        s_manual_start_time = mktime(&manual_time);
    }
    
    return s_ntp_synced ? ESP_OK : ESP_FAIL;
}

/**
 * @brief 检查NTP是否已同步
 */
bool ntp_client_is_synced(void)
{
    return s_ntp_synced;
}

/**
 * @brief 获取当前时间
 */
bool ntp_client_get_time(struct tm *timeinfo)
{
    if (s_ntp_synced) {
        time_t now;
        time(&now);
        return localtime_r(&now, timeinfo) != NULL;
    } else {
        /* 使用手动时间，基于启动时间计算当前时间 */
        static time_t start_time = 0;
        if (start_time == 0) {
            start_time = time(NULL);
        }
        time_t now = s_manual_start_time + (time(NULL) - start_time);
        return localtime_r(&now, timeinfo) != NULL;
    }
}

/**
 * @brief 更新手动时间（备用模式）
 */
void ntp_client_update_manual_time(void)
{
    if (!s_ntp_synced) {
        s_manual_start_time++;
    }
}
