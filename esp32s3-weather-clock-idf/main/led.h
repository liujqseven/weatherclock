/**
 * @file led.h
 * @brief LED驱动头文件
 * @author ESP-IDF Version
 */

#ifndef __LED_H
#define __LED_H

#include "driver/gpio.h"

/* 引脚定义 */
#define LED_PIN       GPIO_NUM_1   /* 开发板上LED连接到GPIO1引脚 */

/* 宏函数定义 */
#define LED_ON()      gpio_set_level(LED_PIN, 0)   /* LED亮（低电平有效） */
#define LED_OFF()     gpio_set_level(LED_PIN, 1)   /* LED灭（高电平熄灭） */
#define LED_TOGGLE()  gpio_set_level(LED_PIN, !gpio_get_level(LED_PIN))

/* 函数声明 */
void led_init(void);      /* led引脚初始化函数 */

#endif
