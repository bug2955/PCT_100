#include "oled_display.h"

// 使用硬件 I2C (HW_I2C) 高速驱动
U8G2_SH1106_128X64_VCOMH0_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void oled_init(void) {
    Wire.begin(I2C_SDA, I2C_SCL); 
    u8g2.begin();
    u8g2.enableUTF8Print(); // 开启 UTF8 支持打印中文
}

void oled_update(String mode, bool masterOn, uint16_t light, uint16_t lightThresh, float temp, float tempThresh, bool lightOn, bool fanOn, bool wifiOn) {
    u8g2.clearBuffer(); // 清空显存

    // ★ 已修改：设置更精细的文泉驿12号中文字体，完美容纳5行排版
    u8g2.setFont(u8g2_font_wqy12_t_gb2312); 

    // ------------------------------------------------
    // 第一行（Y=12）：模式与总闸 (左右分栏对齐)
    // ------------------------------------------------
    u8g2.setCursor(0, 12);
    u8g2.print("模式: ");
    u8g2.print(mode == "auto" ? "自动" : "手动");
    
    u8g2.setCursor(70, 12); 
    u8g2.print("总闸: ");
    u8g2.print(masterOn ? "ON" : "OFF");

    // ------------------------------------------------
    // 第二行（Y=24）：光照数据
    // ------------------------------------------------
    u8g2.setCursor(0, 24);
    u8g2.print("光照: ");
    u8g2.print(light);
    u8g2.print(" / ");
    u8g2.print(lightThresh);

    // ------------------------------------------------
    // 第三行（Y=36）：温度数据
    // ------------------------------------------------
    u8g2.setCursor(0, 36);
    u8g2.print("温度: ");
    u8g2.print(temp, 1); // 保留1位小数
    u8g2.print(" / ");
    u8g2.print(tempThresh, 1);

    // ------------------------------------------------
    // 第四行（Y=48）：继电器状态 (左右分栏对齐)
    // ------------------------------------------------
    u8g2.setCursor(0, 48);
    u8g2.print("灯光: ");
    u8g2.print(lightOn ? "ON" : "OFF");

    u8g2.setCursor(70, 48); 
    u8g2.print("风扇: ");
    u8g2.print(fanOn ? "ON" : "OFF");

    // ------------------------------------------------
    // ★ 第五行（Y=60）：网络状态 (新增行)
    // ------------------------------------------------
    u8g2.setCursor(0, 60);
    u8g2.print("网络: ");
    u8g2.print(wifiOn ? "已联网" : "未联网");

    u8g2.sendBuffer(); // 将显存内容一次性推送到屏幕
}