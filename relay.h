#ifndef __RELAY_H
#define __RELAY_H

#include <Arduino.h>

// 继电器引脚定义
#define RELAY_PIN 2

// 继电器控制宏
#define RELAY_ON()    digitalWrite(RELAY_PIN, HIGH)
#define RELAY_OFF()   digitalWrite(RELAY_PIN, LOW)
#define RELAY_TOGGLE()digitalWrite(RELAY_PIN, !digitalRead(RELAY_PIN))

// 函数声明
void relay_init(void);

#endif