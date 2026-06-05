/**
 * @file PCT_100.ino
 * @brief 系统启动总入口
 * @note 完美契合 ESP32-C3 单核 FreeRTOS 兼容调度架构，彻底实现业务与任务分配解耦
 */
#include <Arduino.h>
#include "src/app_system.h"
#include "src/wifi_mqtt.h"

// =======================================================
// 系统启动初始化
// =======================================================
void setup() {
    Serial.begin(115200);
    
    // 1. 创建跨线程安全数据通信队列 (5包深度，每包256字节)
    mqttPublishQueue = xQueueCreate(5, 256);

    // 2. 初始化核心系统硬件及网络底层状态
    app_system_init();
    wifi_mqtt_init();

    // 3. 启动 FreeRTOS 独立工作线程 (不绑定核心，自适应单核切片调度)
    // 创建【线程一】：网络吞吐任务 (较高优先级 2)
    xTaskCreate(TaskNetwork, "TaskNet", 8192, NULL, 2, NULL);
    
    // 创建【线程二】：RGB 状态指示渲染任务 (最低优先级 0，绝不抢占传感器算力)
    xTaskCreate(TaskLedIndicator, "TaskLED", 2048, NULL, 0, NULL);
}

// =======================================================
// 核心主车道 (Loop / Task 0)
// =======================================================
void loop() {
    // 主线程全身心投入运行主逻辑状态机、消抖以及事件边缘侦测，丝毫不受网络延迟和LED延迟的干扰
    app_system_loop();
    vTaskDelay(pdMS_TO_TICKS(10)); 
}