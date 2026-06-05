#ifndef __OLED_DISPLAY_H
#define __OLED_DISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// 设定引脚
#define I2C_SDA 4
#define I2C_SCL 5

// 函数声明
void oled_init(void);

// ★ 已修改：在参数最后新增了 bool wifiOn，用来接收网络状态
void oled_update(String mode, bool masterOn, uint16_t light, uint16_t lightThresh, float temp, float tempThresh, bool lightOn, bool fanOn, bool wifiOn);

#endif