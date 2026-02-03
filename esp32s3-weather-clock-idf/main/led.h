/**
 * @file led.h
 * @brief LED驱动头文件
 * @author ESP-IDF Version
 */

#ifndef __LED_H
#define __LED_H

#include "driver/gpio.h"

/* 引脚定义 */
#define LED_GPIO_PIN GPIO_NUM_1 /* LED 连接的 GPIO 端口 */

/* 引脚的输出的电平状态 */
enum GPIO_OUTPUT_STATE
{
    PIN_RESET,
    PIN_SET
};

/* LED 端口定义 */
#define LED(x) do { x ? \
    gpio_set_level(LED_GPIO_PIN, PIN_SET) : \
    gpio_set_level(LED_GPIO_PIN, PIN_RESET); \
} while(0) /* LED 控制 */

/* LED 取反定义 */
#define LED_TOGGLE() do { \
    gpio_set_level(LED_GPIO_PIN, !gpio_get_level(LED_GPIO_PIN)); \
} while(0) /* LED 翻转 */

/* 为了兼容现有代码，保留原来的宏定义 */
#define LED_PIN       LED_GPIO_PIN
#define LED_ON()      LED(1)   /* LED亮 */
#define LED_OFF()     LED(0)   /* LED灭 */

/* 函数声明 */
void led_init(void); /* 初始化 LED */

#endif
