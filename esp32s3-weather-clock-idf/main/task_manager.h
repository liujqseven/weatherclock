/**
 * @file task_manager.h
 * @brief 任务管理模块头文件
 * @author ESP-IDF Version
 */

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief 任务类型枚举
 */
typedef enum {
    TASK_TYPE_LED_BLINK,
    TASK_TYPE_WEATHER_UPDATE,
    TASK_TYPE_DISPLAY_UPDATE,
    TASK_TYPE_SYSTEM_HEALTH,
    TASK_TYPE_MAX
} task_type_t;

/**
 * @brief 任务信息结构体
 */
typedef struct {
    TaskHandle_t task_handle;
    const char *task_name;
    UBaseType_t priority;
    uint32_t stack_size;
    bool is_running;
} task_info_t;

/**
 * @brief 初始化任务管理器
 * @return ESP_OK 成功
 * @return 其他错误码 失败
 */
esp_err_t task_manager_init(void);

/**
 * @brief 创建任务
 * @param task_type 任务类型
 * @param task_function 任务函数
 * @param task_params 任务参数
 * @return ESP_OK 成功
 * @return 其他错误码 失败
 */
esp_err_t task_manager_create_task(task_type_t task_type, TaskFunction_t task_function, void *task_params);

/**
 * @brief 删除任务
 * @param task_type 任务类型
 * @return ESP_OK 成功
 * @return 其他错误码 失败
 */
esp_err_t task_manager_delete_task(task_type_t task_type);

/**
 * @brief 重启任务
 * @param task_type 任务类型
 * @param task_function 任务函数
 * @param task_params 任务参数
 * @return ESP_OK 成功
 * @return 其他错误码 失败
 */
esp_err_t task_manager_restart_task(task_type_t task_type, TaskFunction_t task_function, void *task_params);

/**
 * @brief 获取任务信息
 * @param task_type 任务类型
 * @param task_info 任务信息结构体指针
 * @return ESP_OK 成功
 * @return 其他错误码 失败
 */
esp_err_t task_manager_get_task_info(task_type_t task_type, task_info_t *task_info);

/**
 * @brief 检查任务是否运行
 * @param task_type 任务类型
 * @param is_running 运行状态指针
 * @return ESP_OK 成功
 * @return 其他错误码 失败
 */
esp_err_t task_manager_is_task_running(task_type_t task_type, bool *is_running);

#endif /* TASK_MANAGER_H */
