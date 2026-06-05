#ifndef __RELAY_H
#define __RELAY_H

#include <Arduino.h>

// 继电器引脚定义
#define RELAY1_PIN 6  // 灯
#define RELAY2_PIN 7  // 风扇

// 继电器触发电平定义 (根据之前的测试，HIGH应该是开启)
#define RELAY_ON_LEVEL    HIGH
#define RELAY_OFF_LEVEL   LOW

// 函数声明
void relay_init(void);
void relay_set_state(uint8_t state);
void relay_off_all(void);

#endif