#ifndef __RGB_LED_H
#define __RGB_LED_H

#include <Arduino.h>

// 根据原理图，WS2812 连接在 GPIO0
#define RGB_PIN 0      
#define NUM_LEDS 1     // 只有 1 颗灯

void rgb_init(void);
void rgb_set_color(uint8_t r, uint8_t g, uint8_t b);
void rgb_boot_animation(void);

#endif