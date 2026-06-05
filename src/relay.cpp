#include "relay.h"

void relay_init(void)
{
    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);
    relay_off_all();  // 上电默认全部断开
}

// 强制关闭所有继电器
void relay_off_all(void)
{
    digitalWrite(RELAY1_PIN, RELAY_OFF_LEVEL);
    digitalWrite(RELAY2_PIN, RELAY_OFF_LEVEL);
}

// 四态控制机: 0(00), 1(01), 2(10), 3(11)
void relay_set_state(uint8_t state)
{
    switch(state) {
        case 0: // 00: 全关
            digitalWrite(RELAY1_PIN, RELAY_OFF_LEVEL);
            digitalWrite(RELAY2_PIN, RELAY_OFF_LEVEL);
            break;
        case 1: // 01: 仅继电器2(风扇)开
            digitalWrite(RELAY1_PIN, RELAY_OFF_LEVEL);
            digitalWrite(RELAY2_PIN, RELAY_ON_LEVEL);
            break;
        case 2: // 10: 仅继电器1(灯)开
            digitalWrite(RELAY1_PIN, RELAY_ON_LEVEL);
            digitalWrite(RELAY2_PIN, RELAY_OFF_LEVEL);
            break;
        case 3: // 11: 全开
            digitalWrite(RELAY1_PIN, RELAY_ON_LEVEL);
            digitalWrite(RELAY2_PIN, RELAY_ON_LEVEL);
            break;
    }
}