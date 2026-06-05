#include "rgb_led.h"
#include <Adafruit_NeoPixel.h>

// 实例化 NeoPixel 对象
Adafruit_NeoPixel pixels(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

void rgb_init(void) {
    pixels.begin();           // 初始化引脚
    pixels.setBrightness(80); // 设置全局亮度 (0-255)，80 适中不刺眼
    pixels.clear();           // 熄灭
    pixels.show();            // 将数据推送到灯珠
}

// 设定颜色的基础函数
void rgb_set_color(uint8_t r, uint8_t g, uint8_t b) {
    pixels.setPixelColor(0, pixels.Color(r, g, b));
    pixels.show();
}

// 开机跑马灯闪烁动画 (阻塞式，仅在开机执行一次)
void rgb_boot_animation(void) {
    rgb_set_color(255, 0, 0); delay(150); // 红
    rgb_set_color(0, 255, 0); delay(150); // 绿
    rgb_set_color(0, 0, 255); delay(150); // 蓝
    rgb_set_color(0, 0, 0);   delay(150); // 灭
}