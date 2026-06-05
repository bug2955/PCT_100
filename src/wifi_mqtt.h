#ifndef __WIFI_MQTT_H
#define __WIFI_MQTT_H

#include <Arduino.h>

void wifi_mqtt_init(void);                                      
void wifi_mqtt_loop(void);                                      
bool wifi_mqtt_is_connected(void);                              
void wifi_mqtt_publish(const char* topic, const char* payload); 
void wifi_clear_saved_config(void); 
void wifi_mqtt_set_cmd_callback(void (*callback)(String));

// ★ 新增：允许业务层获取当前动态设定的设备 ID
String get_mqtt_client_id(void);

#endif