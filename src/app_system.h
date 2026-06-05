/**
 * @file app_system.h
 * @brief 系统核心业务层头文件
 */
#ifndef __APP_SYSTEM_H
#define __APP_SYSTEM_H

#include <Arduino.h>

// 系统核心生命周期函数
void app_system_init(void);
void app_system_loop(void);

// =======================================================
// ★ FreeRTOS 多线程与队列对外路由接口 (实现主入口解耦)
// =======================================================
extern QueueHandle_t mqttPublishQueue;          // 跨线程通信队列句柄
void TaskNetwork(void *pvParameters);           // 线程一：网络与MQTT专属线程
void TaskLedIndicator(void *pvParameters);      // 线程二：RGB指示灯专项动画线程

#endif