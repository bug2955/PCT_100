#ifndef __TEMP_SENSOR_H
#define __TEMP_SENSOR_H

#include <Arduino.h>

// ★ DS18B20 连接在 IO10
#define TEMP_SENSOR_PIN 10 

// =======================================================
// ★ 温度控制风扇的阈值设定 (加入滞回区间)
// =======================================================
#define TEMP_THRESHOLD_ON  32.0   // 高于 32℃ 判定为过热，开启风扇
#define TEMP_THRESHOLD_OFF 31.0   // 低于 31℃ 判定为凉爽，关闭风扇

void temp_sensor_init(void);
float get_temperature(void);

#endif