#ifndef exti_h
#define exti_h

#include <Arduino.h>

// ==================== ESP32-C3 配置 ====================
#define KEY_PIN             0        // 按键引脚（可任意修改：0~22）
#define DEBOUNCE_MS         20       // 消抖时间 20ms
#define KEY_PRESS_TRIGGER   FALLING  // 上拉模式：按下 = 下降沿

// 初始化函数
void keyInit(void);

// 获取按键触发标志
bool keyPressed(void);

// 清除标志
void keyClear(void);

#endif