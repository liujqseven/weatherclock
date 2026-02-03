/**
 * @file led.c
 * @brief LED驱动实现文件
 * @author ESP-IDF Version
 */

#include "led.h"
#include "esp_log.h"

/**
* @brief       初始化LED相关IO口
* @param       无
* @retval      无
*/
void led_init(void)
{
    gpio_config_t gpio_init_struct = {0};
    
    gpio_init_struct.intr_type = GPIO_INTR_DISABLE; /* 失能引脚中断 */
    gpio_init_struct.mode = GPIO_MODE_INPUT_OUTPUT; /* 输入输出模式 */
    gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE; /* 使能上拉 */
    gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE; /* 失能下拉 */
    gpio_init_struct.pin_bit_mask = 1ull << LED_GPIO_PIN; /* 设置的引脚的位掩码*/
    
    esp_err_t err = gpio_config(&gpio_init_struct);
    if (err == ESP_OK) {
        ESP_LOGI("LED", "LED GPIO initialized successfully on pin %d", LED_GPIO_PIN);
    } else {
        ESP_LOGE("LED", "Failed to initialize LED GPIO: %s", esp_err_to_name(err));
    }
    
    LED(1); /* 关闭 LED */
    ESP_LOGI("LED", "LED initial state: OFF");
}
