/**
 * @file weather_client.c
 * @brief 天气API客户端实现文件
 * @author ESP-IDF Version
 */

#include "weather_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "gzip_decompressor.h"
#include <string.h>

static const char *TAG = "WEATHER_CLIENT";

/* 全局变量 */
static bool s_has_valid_data = false;
static weather_data_t s_weather_data = {0};

/* 事件处理器缓冲区（使用静态变量） */
#define MAX_RESPONSE_BUFFER_SIZE 4096  // 4KB 最大响应缓冲区
static char s_response_buffer[MAX_RESPONSE_BUFFER_SIZE] = {0};
static int s_response_length = 0;

/**
 * @brief 重置响应缓冲区
 */
static void reset_response_buffer(void)
{
    memset(s_response_buffer, 0, MAX_RESPONSE_BUFFER_SIZE);
    s_response_length = 0;
}

/**
 * @brief 解析天气JSON数据
 */
static bool parse_weather_json(const char *json_str, weather_data_t *data)
{
    if (json_str == NULL || data == NULL) {
        return false;
    }
    
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return false;
    }
    
    /* 检查API返回状态码 */
    cJSON *code = cJSON_GetObjectItem(root, "code");
    if (code == NULL || cJSON_GetStringValue(code) == NULL || 
        strcmp(cJSON_GetStringValue(code), "200") != 0) {
        ESP_LOGE(TAG, "API error: %s", code ? cJSON_GetStringValue(code) : "unknown");
        cJSON_Delete(root);
        return false;
    }
    
    /* 提取now对象 */
    cJSON *now = cJSON_GetObjectItem(root, "now");
    if (now == NULL) {
        ESP_LOGE(TAG, "Missing 'now' field in JSON");
        cJSON_Delete(root);
        return false;
    }
    
    /* 提取天气数据 */
    cJSON *temp = cJSON_GetObjectItem(now, "temp");
    if (temp) strncpy(data->temp, cJSON_GetStringValue(temp), sizeof(data->temp) - 1);
    
    cJSON *feelsLike = cJSON_GetObjectItem(now, "feelsLike");
    if (feelsLike) strncpy(data->feelsLike, cJSON_GetStringValue(feelsLike), sizeof(data->feelsLike) - 1);
    
    cJSON *text = cJSON_GetObjectItem(now, "text");
    if (text) strncpy(data->text, cJSON_GetStringValue(text), sizeof(data->text) - 1);
    
    cJSON *windDir = cJSON_GetObjectItem(now, "windDir");
    if (windDir) strncpy(data->windDir, cJSON_GetStringValue(windDir), sizeof(data->windDir) - 1);
    
    cJSON *windScale = cJSON_GetObjectItem(now, "windScale");
    if (windScale) strncpy(data->windScale, cJSON_GetStringValue(windScale), sizeof(data->windScale) - 1);
    
    cJSON *humidity = cJSON_GetObjectItem(now, "humidity");
    if (humidity) strncpy(data->humidity, cJSON_GetStringValue(humidity), sizeof(data->humidity) - 1);
    
    cJSON *vis = cJSON_GetObjectItem(now, "vis");
    if (vis) strncpy(data->vis, cJSON_GetStringValue(vis), sizeof(data->vis) - 1);
    
    /* 提取更新时间 */
    cJSON *updateTime = cJSON_GetObjectItem(root, "updateTime");
    if (updateTime) {
        strncpy(data->updateTime, cJSON_GetStringValue(updateTime), sizeof(data->updateTime) - 1);
    } else {
        strcpy(data->updateTime, "N/A");
    }
    
    cJSON_Delete(root);
    return true;
}

/**
 * @brief HTTP客户端事件处理器
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (evt->data_len > 0) {
                /* 检查缓冲区是否足够 */
                if (s_response_length + evt->data_len + 1 > MAX_RESPONSE_BUFFER_SIZE) {
                    ESP_LOGE(TAG, "Response buffer overflow");
                    reset_response_buffer();
                    return ESP_FAIL;
                }
                
                /* 复制数据到静态缓冲区 */
                memcpy(s_response_buffer + s_response_length, evt->data, evt->data_len);
                s_response_length += evt->data_len;
                s_response_buffer[s_response_length] = '\0';
                
                ESP_LOGD(TAG, "Received %d bytes, total: %d", evt->data_len, s_response_length);
            }
            break;
            
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "Response received: %d bytes", s_response_length);
            break;
            
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP event error");
            reset_response_buffer();
            break;
            
        default:
            break;
    }
    
    return ESP_OK;
}

/**
 * @brief 初始化天气客户端
 */
esp_err_t weather_client_init(void)
{
    ESP_LOGI(TAG, "Initializing weather client...");
    return ESP_OK;
}

/**
 * @brief 从API获取天气数据
 */
esp_err_t weather_client_fetch_data(weather_data_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* 重置缓冲区 */
    reset_response_buffer();
    
    /* 构建API URL */
    char url[512];
    snprintf(url, sizeof(url),
             "https://%s/v7/weather/now?location=%s&key=%s&lang=zh",
             API_HOST, WEATHER_LOCATION_ID, API_KEY);
    
    ESP_LOGI(TAG, "Fetching weather data from: %s", url);
    
    /* 配置HTTP客户端（使用事件处理器） */
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 10000,
        .disable_auto_redirect = true,
        .skip_cert_common_name_check = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = http_event_handler,
    };
    
    /* 创建HTTP客户端 */
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }
    
    /* 设置请求头，接受gzip压缩 */
    esp_http_client_set_header(client, "Accept-Encoding", "gzip");
    esp_http_client_set_header(client, "Accept", "application/json");
    
    /* 执行HTTP请求 */
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        reset_response_buffer();
        return err;
    }
    
    /* 检查HTTP响应状态码 */
    int status_code = esp_http_client_get_status_code(client);
    if (status_code != 200) {
        ESP_LOGE(TAG, "HTTP request failed with status code: %d", status_code);
        esp_http_client_cleanup(client);
        reset_response_buffer();
        return ESP_FAIL;
    }
    
    /* 检查是否收到数据 */
    if (s_response_length == 0) {
        ESP_LOGE(TAG, "Failed to read response body: 0 bytes received");
        esp_http_client_cleanup(client);
        reset_response_buffer();
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Successfully read %d bytes from response", s_response_length);
    
    /* 检查是否是gzip压缩 */
    bool is_gzip = false;
    if (s_response_length >= 2 && 
        (unsigned char)s_response_buffer[0] == 0x1F && 
        (unsigned char)s_response_buffer[1] == 0x8B) {
        ESP_LOGI(TAG, "Response is GZIP compressed (magic bytes: 0x1F 0x8B)");
        is_gzip = true;
    } else {
        ESP_LOGI(TAG, "Response is not GZIP compressed");
    }
    
    /* 检查是否为GZIP压缩数据 */
    char *json_data = NULL;
    size_t json_size = 0;
    
    if (is_gzip) {
        ESP_LOGI(TAG, "Decompressing GZIP data...");
        json_data = gzip_decompress((const uint8_t *)s_response_buffer, s_response_length, &json_size);
        
        if (json_data == NULL) {
            ESP_LOGE(TAG, "Failed to decompress GZIP data");
            esp_http_client_cleanup(client);
            reset_response_buffer();
            return ESP_FAIL;
        }
        
        ESP_LOGI(TAG, "Decompressed JSON size: %d bytes", json_size);
        if (json_size > 0) {
            ESP_LOGD(TAG, "Decompressed JSON: %s", json_data);
        }
    } else {
        /* 不是GZIP压缩，直接使用原始数据 */
        json_data = (char *)malloc(s_response_length + 1);
        if (json_data == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for JSON data");
            esp_http_client_cleanup(client);
            reset_response_buffer();
            return ESP_FAIL;
        }
        memcpy(json_data, s_response_buffer, s_response_length);
        json_data[s_response_length] = '\0';
        json_size = s_response_length;
        ESP_LOGI(TAG, "Using raw response data");
    }
    
    /* 重置响应缓冲区 */
    reset_response_buffer();
    
    /* 解析JSON数据 */
    if (parse_weather_json(json_data, data)) {
        s_has_valid_data = true;
        memcpy(&s_weather_data, data, sizeof(weather_data_t));
        ESP_LOGI(TAG, "Weather data fetched successfully");
        ESP_LOGI(TAG, "Temp: %sC, Condition: %s", data->temp, data->text);
        
        free(json_data);
        esp_http_client_cleanup(client);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to parse weather JSON data");
        free(json_data);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }
}

/**
 * @brief 检查是否有有效的天气数据
 */
bool weather_client_has_valid_data(void)
{
    return s_has_valid_data;
}
