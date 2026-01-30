/**
 * @file wifi_manager.c
 * @brief WiFi连接管理实现文件
 * @author ESP-IDF Version
 */

#include "wifi_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "WIFI_MANAGER";

/* 事件组位定义 */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* 全局变量 */
static EventGroupHandle_t s_wifi_event_group = NULL;
static int s_retry_num = 0;
static wifi_status_t s_wifi_status = WIFI_STATUS_DISCONNECTED;
static wifi_status_callback_t s_status_callback = NULL;

/* WiFi事件处理函数 */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        s_wifi_status = WIFI_STATUS_CONNECTING;
        ESP_LOGI(TAG, "WiFi connecting...");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_CONNECT_TIMEOUT) {
            esp_wifi_connect();
            s_retry_num++;
            s_wifi_status = WIFI_STATUS_CONNECTING;
            ESP_LOGI(TAG, "Retrying to connect to the AP... (%d/%d)", s_retry_num, WIFI_CONNECT_TIMEOUT);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            s_wifi_status = WIFI_STATUS_FAILED;
            ESP_LOGE(TAG, "WiFi connection failed!");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        s_wifi_status = WIFI_STATUS_CONNECTED;
    }
    
    /* 调用回调函数 */
    if (s_status_callback) {
        s_status_callback(s_wifi_status);
    }
}

/**
 * @brief 初始化WiFi连接
 */
esp_err_t wifi_manager_init(void)
{
    esp_err_t ret = ESP_OK;
    
    /* 初始化NVS */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    /* 创建事件组 */
    s_wifi_event_group = xEventGroupCreate();
    
    /* 初始化TCP/IP栈 */
    ESP_ERROR_CHECK(esp_netif_init());
    
    /* 创建默认事件循环 */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    /* 创建WiFi STA接口 */
    esp_netif_create_default_wifi_sta();
    
    /* 配置WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    /* 注册事件处理 */
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    
    /* 配置WiFi参数 */
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi initialization complete");
    
    /* 等待连接完成 */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT * 1000));
    
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to SSID: %s", WIFI_SSID);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to SSID: %s", WIFI_SSID);
        return ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "WiFi connection timeout");
        return ESP_FAIL;
    }
}

/**
 * @brief 获取WiFi连接状态
 */
wifi_status_t wifi_manager_get_status(void)
{
    return s_wifi_status;
}

/**
 * @brief 注册WiFi状态回调函数
 */
void wifi_manager_register_callback(wifi_status_callback_t callback)
{
    s_status_callback = callback;
}

/**
 * @brief 检查WiFi是否已连接
 */
bool wifi_manager_is_connected(void)
{
    return s_wifi_status == WIFI_STATUS_CONNECTED;
}
