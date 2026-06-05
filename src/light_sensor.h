#ifndef __LIGHT_SENSOR_H
#define __LIGHT_SENSOR_H

#include <Arduino.h>

#define LIGHT_SENSOR_PIN 1 

// =======================================================
// ★ 新的阈值设定：范围 0~1000，越暗数值越低
// =======================================================
#define LIGHT_THRESHOLD_ON  250   // 低于 250 判定为过暗，开启照明灯
#define LIGHT_THRESHOLD_OFF 500   // 高于 500 判定为明亮，关闭照明灯

void light_sensor_init(void);
uint16_t get_light_value(void);

#endif