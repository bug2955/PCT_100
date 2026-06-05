#include "exti.h"

// 中断标志（volatile 必须加！）
static volatile bool keyFlag = false;
static unsigned long lastDebounce = 0;

// 中断服务函数（极简！！！）
void IRAM_ATTR keyISR() {
  unsigned long now = millis();
  if (now - lastDebounce > DEBOUNCE_MS) {
    keyFlag = true;
    lastDebounce = now;
  }
}

// 初始化按键 + 中断
void keyInit(void) {
  pinMode(KEY_PIN, INPUT_PULLUP);
  attachInterrupt(KEY_PIN, keyISR, KEY_PRESS_TRIGGER);
}

// 读取按键
bool keyPressed(void) {
  return keyFlag;
}

// 清除标志
void keyClear(void) {
  keyFlag = false;
}