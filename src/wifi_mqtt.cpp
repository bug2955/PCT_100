#include "wifi_mqtt.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h> 
#include "rgb_led.h" 

static String dyn_mqtt_server = "";
static int dyn_mqtt_port = 8081;
static String dyn_mqtt_user = "";
static String dyn_mqtt_pass = "";
static String dyn_client_id = "";
static String dyn_topic_command = ""; 

WiFiClient espClient;
PubSubClient client(espClient);
Preferences prefs;

static void (*app_cmd_callback)(String) = NULL;

enum WifiState {
    WIFI_INIT, WIFI_WAIT_AUTO, WIFI_SCAN, WIFI_WAIT_INDEX, 
    WIFI_WAIT_PASS, WIFI_CONNECTING, WIFI_CONNECTED, WIFI_IDLE          
};
static WifiState wifi_state = WIFI_INIT;
static unsigned long wifi_timer = 0;
static int scan_count = 0;
static int selected_index = -1;
static String target_ssid = "";
static String target_pass = "";
static String saved_ssid = "";
static String saved_pass = "";

static void show_mqtt_config() {
    Serial.println("\n===== 当前 MQTT 配置信息 =====");
    Serial.println("IP地址    : " + dyn_mqtt_server);
    Serial.println("端口号    : " + String(dyn_mqtt_port));
    Serial.println("用户名    : " + dyn_mqtt_user);
    Serial.print("密  码    : ");
    Serial.println(dyn_mqtt_pass.length() > 0 ? "******" : "(未设置)");
    Serial.println("设备 ID   : " + dyn_client_id);
    Serial.println("==============================");
}

static void handle_mqtt_config_cmd(String cmd) {
    bool changed = false;
    if (cmd.startsWith("set_mqtt_ip ")) {
        dyn_mqtt_server = cmd.substring(12);
        prefs.putString("mq_ip", dyn_mqtt_server);
        Serial.println("[配置] MQTT IP 已更新为: " + dyn_mqtt_server);
        changed = true;
    }
    else if (cmd.startsWith("set_mqtt_port ")) {
        dyn_mqtt_port = cmd.substring(14).toInt();
        prefs.putInt("mq_port", dyn_mqtt_port);
        Serial.println("[配置] MQTT 端口已更新为: " + String(dyn_mqtt_port));
        changed = true;
    }
    else if (cmd.startsWith("set_mqtt_user ")) {
        dyn_mqtt_user = cmd.substring(14);
        prefs.putString("mq_user", dyn_mqtt_user);
        Serial.println("[配置] MQTT 用户名已更新为: " + dyn_mqtt_user);
        changed = true;
    }
    else if (cmd.startsWith("set_mqtt_pass ")) {
        dyn_mqtt_pass = cmd.substring(14);
        prefs.putString("mq_pass", dyn_mqtt_pass);
        Serial.println("[配置] MQTT 密码已更新为: ******");
        changed = true;
    }
    else if (cmd.startsWith("set_mqtt_id ")) {
        dyn_client_id = cmd.substring(12);
        prefs.putString("mq_id", dyn_client_id);
        dyn_topic_command = "chemctrl/" + dyn_client_id + "/command";
        Serial.println("[配置] 设备 ID 已更新为: " + dyn_client_id);
        changed = true;
    }

    if (changed) {
        Serial.println("[配置] 参数已保存至 Flash！断开当前 MQTT 连接以应用新配置...");
        client.disconnect(); 
        client.setServer(dyn_mqtt_server.c_str(), dyn_mqtt_port); 
    }
}

static String get_serial_input_nonblocking() {
    static String inputBuffer = "";
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                String result = inputBuffer;
                inputBuffer = "";
                result.trim();

                if (result.startsWith("set_mqtt_")) {
                    handle_mqtt_config_cmd(result);
                    return ""; 
                }
                if (result == "show_mqtt") {
                    show_mqtt_config();
                    return "";
                }

                return result; 
            }
        } else {
            inputBuffer += c;
        }
    }
    return "";
}

void mqtt_callback_handler(char* topic, byte* payload, unsigned int length) {
    String msg = "";
    for (int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    
    if (String(topic) == dyn_topic_command) {
        if (app_cmd_callback != NULL) {
            app_cmd_callback(msg);
        }
    }
}

String get_mqtt_client_id(void) {
    return dyn_client_id;
}

void wifi_mqtt_set_cmd_callback(void (*callback)(String)) {
    app_cmd_callback = callback;
}

void wifi_mqtt_init(void) {
    prefs.begin("wifi_cfg", false);
    
    dyn_mqtt_server = prefs.getString("mq_ip", "");
    dyn_mqtt_port   = prefs.getInt("mq_port", 8081);
    dyn_mqtt_user   = prefs.getString("mq_user", "");
    dyn_mqtt_pass   = prefs.getString("mq_pass", "");
    dyn_client_id   = prefs.getString("mq_id", "");
    
    if (dyn_client_id.length() == 0) {
        dyn_client_id = "PCT_100_" + String(random(1000, 9999));
        Serial.printf("[MQTT] 生成随机设备ID: %s\n", dyn_client_id.c_str());
    }
    
    dyn_topic_command = "chemctrl/" + dyn_client_id + "/command";
    
    if (dyn_mqtt_server.length() > 0) {
        client.setServer(dyn_mqtt_server.c_str(), dyn_mqtt_port);
        Serial.println("[MQTT] 配置已加载，准备连接...");
    } else {
        Serial.println("[MQTT] ⚠️ 未配置 MQTT 服务器，请通过串口命令设置");
        Serial.println("       使用 set_mqtt_ip, set_mqtt_user, set_mqtt_pass 命令配置");
    }
    
    client.setCallback(mqtt_callback_handler); 
    wifi_state = WIFI_INIT;
}

void wifi_mqtt_loop(void) {
    switch (wifi_state) {
        case WIFI_INIT: {
            saved_ssid = prefs.getString("ssid", "");
            saved_pass = prefs.getString("pass", "");
            if (saved_ssid.length() > 0) {
                rgb_set_color(0, 0, 255); 
                Serial.printf("\n[WiFi] 尝试静默连接记忆网络: [%s]\n", saved_ssid.c_str());
                WiFi.begin(saved_ssid.c_str(), saved_pass.c_str());
                wifi_timer = millis();
                wifi_state = WIFI_WAIT_AUTO;
            } else {
                wifi_state = WIFI_SCAN;
            }
            break;
        }
        case WIFI_WAIT_AUTO: {
            if (WiFi.status() == WL_CONNECTED) {
                rgb_set_color(0, 255, 0); 
                Serial.printf("\n[WiFi] 直连 [%s] 成功!\n", saved_ssid.c_str());
                wifi_state = WIFI_CONNECTED;
            } else if (millis() - wifi_timer > 15000) { 
                Serial.println("\n[WiFi] 自动连接失败，进入配网模式！");
                wifi_state = WIFI_SCAN;
            }
            break;
        }
        case WIFI_SCAN: {
            static unsigned long last_scan_time = 0;
            if (millis() - last_scan_time > 5000 || last_scan_time == 0) { 
                last_scan_time = millis();
                WiFi.disconnect(); 
                delay(10);
                rgb_set_color(128, 0, 128); 
                Serial.println("\n====================================");
                Serial.println("[WiFi] 正在扫描附近网络...");
                scan_count = WiFi.scanNetworks();
                if (scan_count > 0) {
                    rgb_set_color(255, 255, 0); 
                    Serial.printf("[WiFi] 找到 %d 个网络：\n", scan_count);
                    for (int i = 0; i < scan_count; ++i) {
                        Serial.printf("  [%d] %s\n", i, WiFi.SSID(i).c_str());
                    }
                    Serial.println(">> 请在输入框输入你要连接的 WiFi 序号, 然后按回车: ");
                    wifi_timer = millis(); 
                    wifi_state = WIFI_WAIT_INDEX;
                } else {
                    rgb_set_color(255, 0, 0); 
                    Serial.println("[WiFi] 未找到网络，稍后重试...");
                }
            }
            break;
        }
        case WIFI_WAIT_INDEX: {
            if (millis() - wifi_timer > 30000) {
                rgb_set_color(255, 0, 0); 
                Serial.println("\n[WiFi] 🕒 等待输入超时，已暂停配网。");
                wifi_state = WIFI_IDLE;
                break;
            }
            String input = get_serial_input_nonblocking();
            if (input.length() > 0) {
                selected_index = input.toInt();
                if (selected_index >= 0 && selected_index < scan_count) {
                    target_ssid = WiFi.SSID(selected_index);
                    if (target_ssid == saved_ssid && saved_pass.length() > 0) {
                        rgb_set_color(0, 0, 255); 
                        Serial.printf("[WiFi] 🔑 免密自动连接 [%s]...\n", target_ssid.c_str());
                        target_pass = saved_pass;
                        WiFi.begin(target_ssid.c_str(), target_pass.c_str());
                        wifi_timer = millis();
                        wifi_state = WIFI_CONNECTING;
                    } else {
                        Serial.print(">> 请输入密码，然后按回车: ");
                        wifi_timer = millis(); 
                        wifi_state = WIFI_WAIT_PASS;
                    }
                } else {
                    Serial.println("\n[错误] 无效的序号！");
                }
            }
            break;
        }
        case WIFI_WAIT_PASS: {
            if (millis() - wifi_timer > 30000) {
                wifi_state = WIFI_IDLE;
                break;
            }
            String input = get_serial_input_nonblocking();
            if (input.length() > 0) {
                target_pass = input;
                rgb_set_color(0, 0, 255); 
                WiFi.begin(target_ssid.c_str(), target_pass.c_str());
                wifi_timer = millis();
                wifi_state = WIFI_CONNECTING;
            }
            break;
        }
        case WIFI_CONNECTING: {
            if (WiFi.status() == WL_CONNECTED) {
                rgb_set_color(0, 255, 0); 
                Serial.printf("\n[WiFi] ★ 成功连接至 [%s]!\n", target_ssid.c_str());
                prefs.putString("ssid", target_ssid);
                prefs.putString("pass", target_pass);
                saved_ssid = target_ssid;
                saved_pass = target_pass;
                WiFi.scanDelete();
                wifi_state = WIFI_CONNECTED;
            } else if (millis() - wifi_timer > 15000) {
                rgb_set_color(255, 0, 0); 
                WiFi.disconnect();
                WiFi.scanDelete();
                wifi_state = WIFI_SCAN;
            }
            break;
        }
        case WIFI_IDLE: {
            static unsigned long last_bg_retry = 0;
            if (saved_ssid.length() > 0 && millis() - last_bg_retry > 15000) {
                last_bg_retry = millis();
                if (WiFi.status() != WL_CONNECTED) WiFi.begin(saved_ssid.c_str(), saved_pass.c_str()); 
            }
            if (saved_ssid.length() > 0 && WiFi.status() == WL_CONNECTED) {
                rgb_set_color(0, 255, 0); 
                Serial.printf("\n\n[WiFi] 🎉 后台自动重连成功！\n");
                wifi_state = WIFI_CONNECTED;
                break;
            }
            String input = get_serial_input_nonblocking();
            if (input.length() > 0) {
                rgb_set_color(128, 0, 128); 
                wifi_state = WIFI_SCAN;
            }
            break;
        }
        case WIFI_CONNECTED: {
            get_serial_input_nonblocking();

            if (WiFi.status() != WL_CONNECTED) {
                rgb_set_color(0, 0, 255); 
                WiFi.reconnect();
                wifi_timer = millis();
                wifi_state = WIFI_WAIT_AUTO;
                break;
            }
            
            if (dyn_mqtt_server.length() > 0) {
                static unsigned long lastReconnectAttempt = 0;
                if (!client.connected()) {
                    unsigned long now = millis();
                    if (now - lastReconnectAttempt > 5000) {
                        lastReconnectAttempt = now;
                        if (client.connect(dyn_client_id.c_str(), dyn_mqtt_user.c_str(), dyn_mqtt_pass.c_str())) {
                            Serial.println("\n[MQTT] 云端服务器已连接!");
                            client.subscribe(dyn_topic_command.c_str());
                        }
                    }
                } else {
                    client.loop(); 
                }
            } else {
                static unsigned long last_notice = 0;
                if (millis() - last_notice > 30000) {
                    last_notice = millis();
                    Serial.println("[MQTT] 等待配置服务器... (使用 set_mqtt_ip 命令设置)");
                }
            }
            break;
        }
    }
}

bool wifi_mqtt_is_connected(void) { return (WiFi.status() == WL_CONNECTED) && client.connected(); }
void wifi_mqtt_publish(const char* topic, const char* payload) { if (wifi_mqtt_is_connected()) client.publish(topic, payload); }
void wifi_clear_saved_config(void) { prefs.begin("wifi_cfg", false); prefs.clear(); }