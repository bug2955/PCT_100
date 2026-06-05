#include "key.h"

void key_init(void) 
{
    // 改用纯 INPUT 模式，避免与外部下拉电阻冲突
    pinMode(KEY1_PIN, INPUT); 
    pinMode(KEY2_PIN, INPUT); 
}