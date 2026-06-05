#ifndef __KEY_H
#define __KEY_H
#include "Arduino.h"

/* 引脚定义 */
#define KEY1_PIN       20  // 总开关 (自锁)
#define KEY2_PIN       21  // 多功能开关 (轻触)

/* 电平逻辑定义 */
// 纯INPUT模式下，假设按下为高电平(1)，松开为低电平(0)
#define KEY_ACTIVE_LEVEL  HIGH

/* 函数声明 */
void key_init(void); 

#endif