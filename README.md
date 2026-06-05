# PCT_100 化工控制实训装置

![License](https://img.shields.io/badge/License-MIT-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Arduino-00979D.svg)
![MCU](https://img.shields.io/badge/MCU-ESP32--C3-blueviolet.svg)
![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)
![Version](https://img.shields.io/badge/Version-V8.0-orange.svg)

> 🎓 **化工自动化专业实训教学平台** —— 基于 ESP32-C3 微控制器的模块化过程控制实训装置

---

## 📋 项目简介

**PCT_100** 是一套专为高校化工自动化、过程控制课程设计的综合性实训平台。通过理论与实践相结合的方式，帮助学生深入理解工业过程控制的核心原理。

### ✨ 核心能力培养

| 能力领域 | 实践内容 |
|----------|----------|
| **传感器技术** | DS18B20 温度采集、BH1750 光照检测、传感器标定 |
| **执行器控制** | 继电器驱动、LED 调光、风扇调速 |
| **通信协议** | I2C、OneWire、UART、MQTT |
| **操作系统** | FreeRTOS 多任务调度、队列通信 |
| **物联网** | WiFi 配网、MQTT 云端接入、远程监控 |
| **控制算法** | 滞回控制、PID 参数整定、状态机设计 |

### 🏆 项目特色

- **模块化设计**：传感器与执行器独立模块，支持自由组合
- **真实工业场景**：模拟化工过程控制中的温度、光照控制回路
- **物联网接入**：支持 MQTT 协议实现远程数据监控
- **FreeRTOS 架构**：真正的多线程实时操作系统实践
- **完整文档支持**：配套产品说明书、安全手册、实训指南

---

## 🎯 核心功能

| 模块 | 功能 | 接口 |
|------|------|------|
| 🌡️ 温度采集 | DS18B20 数字温度传感器 | OneWire (IO10) |
| ☀️ 光照采集 | BH1750 环境光传感器 | I2C |
| ⚡ 继电器控制 | 2 路继电器输出 | IO6 / IO7 |
| 📺 OLED 显示 | 128×64 显示屏 | I2C |
| 🌈 RGB 指示 | WS2812 可编程 LED | IO8 |
| 📶 物联网 | WiFi + MQTT 通信 | ESP32 内置 |

---

## 🏗️ 硬件架构

```
┌─────────────────────────────────────────────────────────────┐
│                    ESP32-C3 主控                            │
│                     (160MHz RISC-V)                        │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐       │
│  │  DS18B20    │   │   BH1750    │   │   OLED      │       │
│  │  温度传感器 │   │   光照传感器 │   │   显示屏    │       │
│  │  OneWire    │   │   I2C       │   │   I2C       │       │
│  └─────────────┘   └─────────────┘   └─────────────┘       │
│         │                  │                  │             │
├─────────┼──────────────────┼──────────────────┼─────────────┤
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐       │
│  │   继电器×2  │   │   RGB LED   │   │   WiFi      │       │
│  │  灯 / 风扇  │   │   WS2812    │   │   MQTT      │       │
│  │  IO6 / IO7  │   │    IO8      │   │   云平台    │       │
│  └─────────────┘   └─────────────┘   └─────────────┘       │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 项目结构

```
PCT_100/
├── PCT_100.ino          # 主程序入口
├── src/                 # Arduino 源代码
│   ├── app_system.cpp/h     # 核心业务逻辑
│   ├── temp_sensor.cpp/h    # DS18B20 温度传感器驱动
│   ├── light_sensor.cpp/h   # BH1750 光照传感器驱动
│   ├── oled_display.cpp/h   # OLED 显示驱动
│   ├── rgb_led.cpp/h        # RGB LED 驱动
│   ├── relay.cpp/h          # 继电器控制
│   ├── key.cpp/h            # 按键处理
│   ├── exti.cpp/h           # 外部中断
│   ├── wifi_mqtt.cpp/h      # WiFi + MQTT 通信
│   └── version.h            # 版本信息
├── website/             # 网站文档
│   ├── index.html           # 产品说明书
│   ├── safety.html          # 化工安全知识
│   ├── projects.html        # 实训项目展示
│   └── styles.css           # 网站样式
├── docs/                # 模块说明文档
├── README.md            # 项目说明
├── LICENSE              # MIT 许可证
└── .gitignore           # Git 忽略规则
```

---

## 🚀 快速上手

### 硬件连接

| 模块 | 引脚 | 说明 |
|------|------|------|
| DS18B20 | IO10 | 需接 4.7kΩ 上拉电阻 |
| BH1750 | I2C | SDA: IO4, SCL: IO5 |
| OLED | I2C | SDA: IO4, SCL: IO5 |
| 继电器1 | IO6 | 控制灯 |
| 继电器2 | IO7 | 控制风扇 |
| RGB LED | IO8 | WS2812 数据引脚 |

### 软件配置

1. **安装依赖库**：
   - `OneWire` (Paul Stoffregen)
   - `DallasTemperature`
   - `Wire`
   - `Adafruit SSD1306`
   - `Adafruit GFX Library`
   - `FastLED`
   - `PubSubClient`
   - `ArduinoJson`

2. **WiFi 配网**：
   ```bash
   # 通过串口发送命令（115200 baud）
   # 扫描网络并选择连接
   ```

3. **MQTT 配置**：
   ```bash
   set_mqtt_ip <服务器地址>
   set_mqtt_user <用户名>
   set_mqtt_pass <密码>
   set_mqtt_id <设备ID>
   ```

---

## 📚 实训项目

| 项目 | 难度 | 内容 |
|------|------|------|
| 项目一 | 🟢 入门 | 温度采集与风扇控制 |
| 项目二 | 🟢 入门 | 光照采集与灯控 |
| 项目三 | 🟡 中级 | OLED 显示与 HMI |
| 项目四 | 🟡 中级 | WiFi 与 MQTT 物联网 |
| 项目五 | 🔴 高级 | FreeRTOS 多线程编程 |
| 项目六 | 🔴 高级 | 多回路综合控制 |

---

## 🌐 网站文档

| 文档类型 | 链接 |
|----------|------|
| **产品说明书** | https://bug2955.github.io/PCT_100/ |
| **化工安全知识** | https://bug2955.github.io/PCT_100/website/safety.html |
| **实训项目展示** | https://bug2955.github.io/PCT_100/website/projects.html |

---

## 🎓 适用场景

### 👨‍🎓 教学实训
- **课程实验**：过程控制原理、工业自动化、传感器技术
- **毕业设计**：基于物联网的智能监控系统设计
- **技能竞赛**：嵌入式系统开发、物联网应用

### 🏭 工业模拟
- **温度控制**：模拟反应釜温度调节
- **光照控制**：模拟车间照明自动控制
- **环境监测**：温湿度、光照多参数监测

### 🔧 开发学习
- **ESP32 入门**：熟悉 ESP32-C3 开发环境
- **FreeRTOS 实践**：多任务编程入门
- **MQTT 物联网**：云端数据上报与远程控制

---

## 📝 代码规范

- 使用 `snake_case` 命名变量和函数
- 函数前添加 Doxygen 风格注释
- 常量定义使用 `UPPER_CASE`
- 代码行宽不超过 80 字符

---

## 📄 许可证

MIT License - 详见 [LICENSE](LICENSE)

---

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

---

*版本：V8.0 | 设备 ID：PCT_100_014*