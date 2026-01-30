/**
 * @file led.c
 * @brief LED驱动实现文件
 * @author ESP-IDF Version
 */

#include "led.h"

/**
* @brief       初始化LED相关IO口
* @param       无
* @retval      无
*/
void led_init(void)
{
    gpio_config_t io_conf = {0};
    
    io_conf.intr_type = GPIO_INTR_DISABLE;  // 禁用中断
    io_conf.mode = GPIO_MODE_OUTPUT;        // 设置为输出模式
    io_conf.pin_bit_mask = (1ULL << LED_PIN); // 配置LED_PIN引脚
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // 禁用下拉
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;      // 使能上拉
    
    gpio_config(&io_conf);
    
    LED_OFF();  // 初始状态：LED熄灭
}
