/**
 * @file app_system.cpp
 * @brief 业务逻辑中心、事件侦测引擎、FreeRTOS任务体、RGB实时渲染打断引擎
 */
#include "app_system.h"
#include "key.h"
#include "relay.h"
#include "version.h"
#include "light_sensor.h"  
#include "temp_sensor.h"   
#include "oled_display.h"  
#include "wifi_mqtt.h"  
#include "rgb_led.h"       
#include <ArduinoJson.h> 
#include <WiFi.h>

// =======================================================
// [业务层状态变量]
// =======================================================
static String currentMode = "auto";    
static uint8_t relayState = 0;         
static bool systemEnabled = false;     

static unsigned long pressStartTime = 0;
static bool key2Pressed = false;
static bool longPressHandled = false;
static unsigned long lastOledTime = 0; 
static unsigned long lastPublishTime = 0;

static float dynamic_temp_thresh = TEMP_THRESHOLD_ON;
static uint16_t dynamic_light_thresh = LIGHT_THRESHOLD_ON;

// =======================================================
// 模块 1：状态打包投递 (推入 FreeRTOS 队列，不直接发送)
// =======================================================
void report_device_status() {
    if (!wifi_mqtt_is_connected()) return;
    
    char jsonBuffer[256];
    bool r3_light = (digitalRead(RELAY1_PIN) == RELAY_ON_LEVEL);
    bool r4_fan   = (digitalRead(RELAY2_PIN) == RELAY_ON_LEVEL);

    // ★ 修复老师新版大屏：在 JSON 末尾追加当前阈值参数，让大屏摆脱 "--" 显示
    sprintf(jsonBuffer, "{\"temperature\":%.1f,\"light\":%d,\"mode\":\"%s\",\"key1_lock\":%s,\"relay3\":%s,\"relay4\":%s,\"temp_threshold\":%.1f,\"light_threshold\":%d}",
            get_temperature(), 
            get_light_value(), 
            currentMode.c_str(), 
            systemEnabled ? "true" : "false",
            r3_light ? "true" : "false",
            r4_fan ? "true" : "false",
            dynamic_temp_thresh,        
            dynamic_light_thresh);      

    // 塞入全局通信队列，交由线程一去统一投递
    if (mqttPublishQueue != NULL) {
        xQueueSend(mqttPublishQueue, jsonBuffer, (TickType_t)0);
    }
}

// =======================================================
// 模块 2：远程命令接收回调
// =======================================================
void handle_mqtt_command(String payload) {
    Serial.println("\n[指令接收] 收到服务器下发指令: " + payload);

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.print("[错误] JSON 解析失败: ");
        Serial.println(error.c_str());
        return;
    }

    String cmd = doc["cmd"];

    if (cmd == "reboot") {
        Serial.println("[执行] 收到远程重启指令，正在重启...");
        delay(500);
        ESP.restart();
    }
    else if (cmd == "get_status") {
        Serial.println("[执行] 收到状态查询指令，立即上报...");
        report_device_status();
    }
    else if (cmd == "set_threshold") {
        if (doc.containsKey("temp"))  dynamic_temp_thresh = doc["temp"];
        if (doc.containsKey("light")) dynamic_light_thresh = doc["light"];
        Serial.printf("[执行] 阈值已更新 -> 温度: %.1f, 光照: %d\n", dynamic_temp_thresh, dynamic_light_thresh);
    }
    else if (cmd == "set_mode") {
        if (!systemEnabled) {
            Serial.println("[拦截] 总闸关闭中，拒绝切换模式！");
            return;
        }
        String new_mode = doc["mode"];
        if (new_mode == "auto" || new_mode == "manual") {
            currentMode = new_mode;
            relayState = 0;
            relay_off_all(); 
            Serial.println("[执行] 模式已切换为: " + currentMode);
        }
    }
    else if (cmd == "set_relay") {
        if (!systemEnabled) {
            Serial.println("[拦截] 总闸关闭中，拒绝控制继电器！");
            return;
        }
        int relayNum = doc["relay"];
        bool val = doc["value"];
        
        bool r3_light = (digitalRead(RELAY1_PIN) == RELAY_ON_LEVEL);
        bool r4_fan   = (digitalRead(RELAY2_PIN) == RELAY_ON_LEVEL);
        
        if (relayNum == 3) r3_light = val;
        if (relayNum == 4) r4_fan = val;
        
        relayState = 0;
        if (r4_fan)   relayState += 1;
        if (r3_light) relayState += 2;
        
        relay_set_state(relayState);
        Serial.printf("[执行] 继电器 %d 状态已设为 %s\n", relayNum, val ? "ON" : "OFF");
    }
}

// =======================================================
// 模块 3：系统硬件初始化
// =======================================================
void app_system_init(void) {
    key_init();
    relay_init();
    light_sensor_init(); 
    temp_sensor_init();  
    oled_init();         
    rgb_init();          
    
    oled_update("配网中", false, get_light_value(), dynamic_light_thresh, get_temperature(), dynamic_temp_thresh, false, false, false);
    rgb_boot_animation();

    relayState = 0;
    relay_off_all();
    
    wifi_mqtt_set_cmd_callback(handle_mqtt_command);
    Serial.println("\n系统初始化完毕，当前状态：[自动模式]");
}

// =======================================================
// 模块 4：业务核心主状态机 (跑在 Loop 主线程)
// =======================================================
void app_system_loop(void) {
    // A. KEY1 总闸状态判断
    bool isKey1On = (digitalRead(KEY1_PIN) == KEY_ACTIVE_LEVEL); 
    if (!isKey1On) {
        if (systemEnabled) {
            relay_off_all();    
            relayState = 0;     
            systemEnabled = false;
            key2Pressed = false; 
            longPressHandled = false;
            Serial.println("[状态] 总闸已关闭，继电器断开，进入锁定状态");
        }
    } else {
        if (!systemEnabled) {
            systemEnabled = true;
            currentMode = "auto"; 
            Serial.println("[状态] 总闸已开启，系统准备就绪，已恢复至: 【自动模式】");
        }
    }

    // B. KEY2 长短按消抖逻辑
    if (systemEnabled) { 
        static bool lastRawState = false;       
        static bool stableState = false;        
        static unsigned long lastDebounceTime = 0; 
        
        bool rawState = (digitalRead(KEY2_PIN) == KEY_ACTIVE_LEVEL);
        if (rawState != lastRawState) lastDebounceTime = millis();
        
        if ((millis() - lastDebounceTime) > 30) {
            if (rawState != stableState) {
                stableState = rawState;
                if (stableState == true) {
                    pressStartTime = millis();
                    longPressHandled = false; 
                } 
                else {
                    if (!longPressHandled) {
                        if (currentMode == "manual") {
                            relayState = (relayState + 1) % 4; 
                            relay_set_state(relayState);
                            Serial.printf("[手动操作] 短按触发！继电器状态切换至: %d\n", relayState);
                        } else {
                            Serial.println("[无效操作] 自动模式下短按被屏蔽！");
                        }
                    }
                }
            }
        }
        
        if (stableState == true && !longPressHandled) {
            if (millis() - pressStartTime >= 2000) {
                longPressHandled = true; 
                currentMode = (currentMode == "auto") ? "manual" : "auto";
                relayState = 0;
                relay_off_all();
                Serial.println("\n>>> 已切换至: 【" + currentMode + "模式】 <<<");
            }
        }
        lastRawState = rawState;
    }

    // C. 自动传感控制逻辑 (跟随式滞回控制区)
    static unsigned long lastSensorCheck = 0;
    if (systemEnabled && currentMode == "auto" && (millis() - lastSensorCheck > 500)) {
        lastSensorCheck = millis(); 
        
        uint16_t currentLight = get_light_value();
        if (currentLight < dynamic_light_thresh) {
            digitalWrite(RELAY1_PIN, RELAY_ON_LEVEL);  
        } else if (currentLight > (dynamic_light_thresh + 250)) { 
            digitalWrite(RELAY1_PIN, RELAY_OFF_LEVEL); 
        }

        float currentTemp = get_temperature();
        if (currentTemp > -100.0) { 
            if (currentTemp > dynamic_temp_thresh) {
                digitalWrite(RELAY2_PIN, RELAY_ON_LEVEL);  
            } else if (currentTemp < (dynamic_temp_thresh - 1.0)) { 
                digitalWrite(RELAY2_PIN, RELAY_OFF_LEVEL); 
            }
        }
    }

    // D. OLED 屏幕非阻塞规律刷新 (200ms)
    unsigned long now = millis();
    if (now - lastOledTime > 200) {
        lastOledTime = now;
        bool isLightOn = (digitalRead(RELAY1_PIN) == RELAY_ON_LEVEL); 
        bool isFanOn = (digitalRead(RELAY2_PIN) == RELAY_ON_LEVEL); 

        oled_update(
            currentMode, systemEnabled, 
            get_light_value(), dynamic_light_thresh, 
            get_temperature(), dynamic_temp_thresh, 
            isLightOn, isFanOn, wifi_mqtt_is_connected() 
        );
    }

    // E. ★ 满足老师硬核要求：事件变化侦测引擎 + 突发立即主动上报
    static uint8_t last_relayState = 255;
    static String last_mode = "";
    static bool last_temp_trigger = false;
    static bool last_light_trigger = false;

    bool current_temp_trigger = (get_temperature() > dynamic_temp_thresh);
    bool current_light_trigger = (get_light_value() < dynamic_light_thresh);

    if (relayState != last_relayState || currentMode != last_mode ||
        current_temp_trigger != last_temp_trigger || current_light_trigger != last_light_trigger) {
        
        report_device_status(); // 突发状态，1毫秒内捕获并推进队列！
        
        last_relayState = relayState;
        last_mode = currentMode;
        last_temp_trigger = current_temp_trigger;
        last_light_trigger = current_light_trigger;
    }

    // 5秒保底心跳，维持大屏在线状态
    if (now - lastPublishTime > 5000) {
        lastPublishTime = now;
        report_device_status(); 
    }
}

// =========================================================================
// =========================================================================
// ★ 核心重构区：FreeRTOS 独立多线程底层实现体 (彻底抛离主文件)
// =========================================================================
// =========================================================================

QueueHandle_t mqttPublishQueue = NULL; 

// 动态 LED 状态优先级判定器
int get_current_led_state() {
    bool r3_light = (digitalRead(RELAY1_PIN) == RELAY_ON_LEVEL); 
    bool r4_fan   = (digitalRead(RELAY2_PIN) == RELAY_ON_LEVEL); 
    bool wifi_conn = (WiFi.status() == WL_CONNECTED);
    bool mqtt_conn = wifi_mqtt_is_connected(); 

    if (r3_light && r4_fan) return 1; // 1. 严重警报 (双重越界)
    if (r3_light) return 2;           // 2. 光照越界
    if (r4_fan) return 3;             // 3. 温度越界
    if (!wifi_conn) return 5;         // 5. WiFi未连接
    if (!mqtt_conn) return 4;         // 4. MQTT未连接
    return 6;                         // 6. 所有状态正常
}

// 防阻塞延时器：一旦状态改变，在20ms内强制打破当前动画
bool led_delay(int expected_state, int ms) {
    int steps = ms / 20; 
    int rem = ms % 20;
    for(int i = 0; i < steps; i++) {
        if (get_current_led_state() != expected_state) return false; 
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    if (rem > 0) {
        if (get_current_led_state() != expected_state) return false;
        vTaskDelay(pdMS_TO_TICKS(rem));
    }
    return true;
}

// 色彩平滑渐变器：将 32位 颜色平滑流转过渡，支持实时切断
bool led_fade(int expected_state, uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2, int ms) {
    int steps = ms / 20; 
    if(steps == 0) steps = 1;
    for(int i = 1; i <= steps; i++) {
        if (get_current_led_state() != expected_state) return false; 
        uint8_t r = r1 + (r2 - r1) * i / steps;
        uint8_t g = g1 + (g2 - g1) * i / steps;
        uint8_t b = b1 + (b2 - b1) * i / steps;
        rgb_set_color(r, g, b); 
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    return true; 
}

// 【线程一】：网络与 MQTT 专属线程 (高优先级 2)
void TaskNetwork(void *pvParameters) {
    char jsonBuffer[256];
    for(;;) {
        wifi_mqtt_loop(); 
        if (xQueueReceive(mqttPublishQueue, jsonBuffer, 0) == pdTRUE) {
            String dynamic_topic_status = "chemctrl/" + get_mqtt_client_id() + "/status";
            wifi_mqtt_publish(dynamic_topic_status.c_str(), jsonBuffer);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

// 【线程二】：RGB 状态指示灯独立动画线程 (满足全部考核点，最低优先级 0)
void TaskLedIndicator(void *pvParameters) {
    for(;;) {
        int current_state = get_current_led_state();

        switch(current_state) {
            case 1: // 严重警报 (红红绿绿蓝蓝快闪)
                rgb_set_color(255, 0, 0); if(!led_delay(1, 100)) break;
                rgb_set_color(0, 0, 0);   if(!led_delay(1, 50)) break;
                rgb_set_color(255, 0, 0); if(!led_delay(1, 100)) break;
                rgb_set_color(0, 0, 0);   if(!led_delay(1, 50)) break;
                rgb_set_color(0, 255, 0); if(!led_delay(1, 100)) break;
                rgb_set_color(0, 0, 0);   if(!led_delay(1, 50)) break;
                rgb_set_color(0, 255, 0); if(!led_delay(1, 100)) break;
                rgb_set_color(0, 0, 0);   if(!led_delay(1, 50)) break;
                rgb_set_color(0, 0, 255); if(!led_delay(1, 100)) break;
                rgb_set_color(0, 0, 0);   if(!led_delay(1, 50)) break;
                rgb_set_color(0, 0, 255); if(!led_delay(1, 100)) break;
                rgb_set_color(0, 0, 0);   if(!led_delay(1, 350)) break;
                break;

            case 2: // 光照报警 (红绿蓝快闪)
                rgb_set_color(255, 0, 0); if(!led_delay(2, 300)) break;
                rgb_set_color(0, 0, 0);   if(!led_delay(2, 200)) break;
                rgb_set_color(0, 255, 0); if(!led_delay(2, 300)) break;
                rgb_set_color(0, 0, 0);   if(!led_delay(2, 200)) break;
                rgb_set_color(0, 0, 255); if(!led_delay(2, 300)) break;
                rgb_set_color(0, 0, 0);   if(!led_delay(2, 200)) break;
                break;

            case 3: // 温度报警 (慢流转长闪)
                rgb_set_color(255, 0, 0); if(!led_delay(3, 500)) break;
                rgb_set_color(0, 255, 0); if(!led_delay(3, 500)) break;
                rgb_set_color(0, 0, 255); if(!led_delay(3, 500)) break;
                rgb_set_color(0, 0, 0);   if(!led_delay(3, 700)) break;
                break;

            case 4: // MQTT未连接 (平滑变色呼吸灯)
                rgb_set_color(255, 0, 0); 
                if(!led_fade(4, 255,0,0, 0,255,0, 500)) break; 
                if(!led_fade(4, 0,255,0, 0,0,0,   500)) break; 
                if(!led_fade(4, 0,0,0,   0,0,255, 500)) break; 
                if(!led_fade(4, 0,0,255, 0,0,0,   500)) break; 
                break;

            case 5: // WiFi未连接 (红绿交替闪烁)
                rgb_set_color(255, 0, 0); if(!led_delay(5, 200)) break;
                rgb_set_color(0, 255, 0); if(!led_delay(5, 200)) break;
                rgb_set_color(0, 0, 0);   if(!led_delay(5, 500)) break;
                break;

            case 6: // 全部正常 (全彩RGB渐变呼吸流转)
                rgb_set_color(0, 0, 0); 
                if(!led_fade(6, 0,0,0,   255,0,0, 1000)) break; 
                if(!led_fade(6, 255,0,0, 0,255,0, 1000)) break; 
                if(!led_fade(6, 0,255,0, 0,0,255, 1000)) break; 
                if(!led_fade(6, 0,0,255, 0,0,0,   1000)) break; 
                if(!led_delay(6, 200)) break;                   
                break;
        }
    }
}