/**
 * @file task_manager.c
 * @brief 任务管理模块实现
 * @author ESP-IDF Version
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "task_manager.h"

static const char *TAG = "TASK_MANAGER";

/* 任务信息数组 */
static task_info_t s_task_info[TASK_TYPE_MAX] = {
    {
        .task_handle = NULL,
        .task_name = "led_blink",
        .priority = 3,
        .stack_size = 2048,
        .is_running = false
    },
    {
        .task_handle = NULL,
        .task_name = "weather_update",
        .priority = 5,
        .stack_size = 16384,
        .is_running = false
    },
    {
        .task_handle = NULL,
        .task_name = "display_update",
        .priority = 4,
        .stack_size = 4096,
        .is_running = false
    },
    {
        .task_handle = NULL,
        .task_name = "system_health",
        .priority = 1,
        .stack_size = 2048,
        .is_running = false
    }
};

/**
 * @brief 初始化任务管理器
 */
esp_err_t task_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing task manager");
    
    /* 初始化任务信息 */
    for (int i = 0; i < TASK_TYPE_MAX; i++) {
        s_task_info[i].task_handle = NULL;
        s_task_info[i].is_running = false;
    }
    
    ESP_LOGI(TAG, "Task manager initialized successfully");
    return ESP_OK;
}

/**
 * @brief 创建任务
 */
esp_err_t task_manager_create_task(task_type_t task_type, TaskFunction_t task_function, void *task_params)
{
    if (task_type >= TASK_TYPE_MAX) {
        ESP_LOGE(TAG, "Invalid task type: %d", task_type);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (task_function == NULL) {
        ESP_LOGE(TAG, "Invalid task function");
        return ESP_ERR_INVALID_ARG;
    }
    
    /* 检查任务是否已经存在 */
    if (s_task_info[task_type].is_running) {
        ESP_LOGW(TAG, "Task %s is already running", s_task_info[task_type].task_name);
        return ESP_OK;
    }
    
    ESP_LOGD(TAG, "Creating task %s with stack size %" PRIu32 " and priority %u", 
             s_task_info[task_type].task_name, 
             s_task_info[task_type].stack_size, 
             s_task_info[task_type].priority);
    
    /* 创建任务 */
    BaseType_t result = xTaskCreate(task_function, 
                                   s_task_info[task_type].task_name, 
                                   s_task_info[task_type].stack_size, 
                                   task_params, 
                                   s_task_info[task_type].priority, 
                                   &s_task_info[task_type].task_handle);
    
    if (result == pdPASS) {
        s_task_info[task_type].is_running = true;
        ESP_LOGI(TAG, "Task %s created successfully", s_task_info[task_type].task_name);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to create task %s: %d", s_task_info[task_type].task_name, result);
        return ESP_FAIL;
    }
}

/**
 * @brief 删除任务
 */
esp_err_t task_manager_delete_task(task_type_t task_type)
{
    if (task_type >= TASK_TYPE_MAX) {
        ESP_LOGE(TAG, "Invalid task type: %d", task_type);
        return ESP_ERR_INVALID_ARG;
    }
    
    /* 检查任务是否存在 */
    if (!s_task_info[task_type].is_running || s_task_info[task_type].task_handle == NULL) {
        ESP_LOGW(TAG, "Task %s is not running", s_task_info[task_type].task_name);
        return ESP_OK;
    }
    
    ESP_LOGD(TAG, "Deleting task %s", s_task_info[task_type].task_name);
    
    /* 删除任务 */
    vTaskDelete(s_task_info[task_type].task_handle);
    
    /* 更新任务状态 */
    s_task_info[task_type].task_handle = NULL;
    s_task_info[task_type].is_running = false;
    
    ESP_LOGI(TAG, "Task %s deleted successfully", s_task_info[task_type].task_name);
    return ESP_OK;
}

/**
 * @brief 重启任务
 */
esp_err_t task_manager_restart_task(task_type_t task_type, TaskFunction_t task_function, void *task_params)
{
    if (task_type >= TASK_TYPE_MAX) {
        ESP_LOGE(TAG, "Invalid task type: %d", task_type);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (task_function == NULL) {
        ESP_LOGE(TAG, "Invalid task function");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "Restarting task %s", s_task_info[task_type].task_name);
    
    /* 先删除任务 */
    esp_err_t err = task_manager_delete_task(task_type);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to delete task %s: %s", s_task_info[task_type].task_name, esp_err_to_name(err));
        return err;
    }
    
    /* 再创建任务 */
    err = task_manager_create_task(task_type, task_function, task_params);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create task %s: %s", s_task_info[task_type].task_name, esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "Task %s restarted successfully", s_task_info[task_type].task_name);
    return ESP_OK;
}

/**
 * @brief 获取任务信息
 */
esp_err_t task_manager_get_task_info(task_type_t task_type, task_info_t *task_info)
{
    if (task_type >= TASK_TYPE_MAX) {
        ESP_LOGE(TAG, "Invalid task type: %d", task_type);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (task_info == NULL) {
        ESP_LOGE(TAG, "Invalid task info pointer");
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(task_info, &s_task_info[task_type], sizeof(task_info_t));
    return ESP_OK;
}

/**
 * @brief 检查任务是否运行
 */
esp_err_t task_manager_is_task_running(task_type_t task_type, bool *is_running)
{
    if (task_type >= TASK_TYPE_MAX) {
        ESP_LOGE(TAG, "Invalid task type: %d", task_type);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (is_running == NULL) {
        ESP_LOGE(TAG, "Invalid is_running pointer");
        return ESP_ERR_INVALID_ARG;
    }
    
    *is_running = s_task_info[task_type].is_running;
    return ESP_OK;
}
