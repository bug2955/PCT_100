#include "light_sensor.h"

void light_sensor_init(void) {
    pinMode(LIGHT_SENSOR_PIN, INPUT);
}

// 获取光照处理后的数值 (范围 0 - 1000)
uint16_t get_light_value(void) {
    uint16_t raw_adc = analogRead(LIGHT_SENSOR_PIN); 
    
    // ★ 核心魔法：使用 map 函数
    // 硬件原本：0 (最亮) -> 4095 (最暗)
    // 映射目标：1000 (最亮) -> 0 (最暗)
    uint16_t mapped_val = map(raw_adc, 0, 4095, 1000, 0);
    
    return mapped_val; 
}