# ESP32-S3 天气时钟系统 (ESP-IDF 版本)

基于正点原子ESP32S3开发板的智能天气时钟项目，采用ESP-IDF v5.3.1框架开发。

## 📋 项目概述

这是一个完整的物联网应用示例，集成WiFi连接、NTP时间同步、实时天气数据获取和TFT显示功能。

## ✨ 功能特性

- ✅ **WiFi连接管理** - 自动连接、断线重连
- ✅ **NTP时间同步** - 双服务器冗余
- ✅ **实时天气数据** - 和风天气API v7接口，每30分钟自动更新
- ✅ **HTTPS安全通信** - 证书验证、TLS加密
- ✅ **GZIP压缩支持** - 减少数据传输量
- ✅ **TFT显示驱动** - 80x160竖屏模式，16位真彩色
- ✅ **LED状态指示** - 5秒闪烁，系统运行状态可视化
- ✅ **优化显示刷新** - 时间显示只更新变化部分，减少闪烁
- ✅ **黑色背景显示** - LCD背景黑色，字体清晰可见
- ✅ **多语言支持** - 天气状况和风向中英文转换

## 🔧 硬件平台

### 硬件配置
| 组件 | 规格 |
|------|------|
| **主控芯片** | ESP32-S3 |
| **Flash** | 16MB |
| **显示屏** | 0.9英寸TFT LCD (80x160) |
| **LED** | GPIO1 |

### 引脚分配
```
TFT LCD 引脚连接：
- SCLK: GPIO14
- MOSI: GPIO13
- CS:   GPIO15
- DC:   GPIO2
- RST:  GPIO3
- BL:   GPIO4

LED 引脚：
- LED:  GPIO1 (高电平有效)
```

## 📁 项目结构

```
esp32s3-weather-clock-idf/
├── CMakeLists.txt
├── sdkconfig
├── main/
│   ├── main.c                  # 主应用程序入口
│   ├── led.c/h                 # LED驱动模块
│   ├── wifi_manager.c/h        # WiFi连接管理模块
│   ├── ntp_client.c/h          # NTP时间同步模块
│   ├── weather_client.c/h      # 天气API客户端模块
│   ├── gzip_decompressor.c/h   # GZIP解压缩模块
│   ├── display_manager.c/h     # 显示管理模块
│   ├── system_init.c/h         # 系统初始化模块
│   ├── task_manager.c/h        # 任务管理模块
│   └── error_handler.c/h       # 错误处理模块
└── components/
    ├── BSP/                    # 板级支持包
    │   ├── LCD/                # LCD驱动
    │   ├── LED/                # LED驱动
    │   └── SPI/                # SPI驱动
    └── zlib/                   # ZLIB压缩库
```

## 🚀 快速开始

### 1. 环境准备

确保已安装ESP-IDF v5.x或更高版本。

### 2. 配置WiFi和API密钥

复制配置文件：
```bash
cp main/config_example.h main/config.h
```

编辑 `main/config.h`：
```c
// 和风天气API配置
#define WEATHER_API_HOST "devapi.qweatherapi.com"
#define WEATHER_API_KEY "your_api_key_here"
#define LOCATION_ID "101020100"  // 上海

// WiFi网络信息
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
```

**获取API密钥：**
访问 [和风天气官网](https://dev.qweather.com/) 注册账号并获取免费API密钥。

### 3. 编译和烧录

```bash
# 进入项目目录
cd esp32s3-weather-clock-idf

# 配置项目（可选）
idf.py menuconfig

# 编译项目
idf.py build

# 烧录到ESP32-S3（替换COMx为你的串口）
idf.py -p COMx flash monitor

# 快捷键：Ctrl+] 退出monitor
```

### 4. 预期输出

烧录成功后，串口监视器将显示：
```
I (xxxx) MAIN_APP: === ESP32-S3 天气时钟系统启动 ===
I (xxxx) WIFI_MANAGER: WiFi connecting...
I (xxxx) WIFI_MANAGER: Connected to SSID: your_wifi_ssid
I (xxxx) WIFI_MANAGER: WiFi connected! IP: 192.168.77.81
I (xxxx) NTP_CLIENT: NTP time synchronized!
I (xxxx) NTP_CLIENT: Current time: 2026-02-13 14:42:34
I (xxxx) WEATHER_CLIENT: Weather data fetched successfully
I (xxxx) WEATHER_CLIENT: Temp: 18C, Condition: 阴
I (xxxx) MAIN_APP: === 系统初始化完成 ===
```

## 📖 模块说明

### 主要模块

- **LED驱动模块** - 控制GPIO1上的LED指示灯
- **WiFi管理器模块** - 管理WiFi连接、断线重连、状态监控
- **NTP客户端模块** - 同步网络时间，提供时间格式化接口
- **天气客户端模块** - 从和风天气API获取实时天气数据
- **GZIP解压缩模块** - 使用zlib库解压缩GZIP格式数据
- **显示管理模块** - 管理TFT LCD显示屏，显示天气和时间信息
- **系统初始化模块** - 统一管理所有系统模块的初始化
- **任务管理模块** - 管理系统所有任务的创建、删除和重启
- **错误处理模块** - 统一管理系统错误处理和日志

### 显示布局

```
┌─────────────────────────────────┐
│ 第1行: 时间 (14:42:34)          │ [白色]
│ 第2行: 日期 (2026-02-13)        │ [白色]
│ 第3行: 温度 (Temp: 18C)         │ [蓝色标签/红色数值]
│ 第4行: 天气状况 (WX: OVERCAST)  │ [蓝色标签/红色数值]
│ 第5行: 体感温度 (Feel: 17C)     │ [蓝色标签/红色数值]
│ 第6行: 风向 (Wind: N)           │ [蓝色标签/红色数值]
│ 第7行: 风力等级 (Spd: 3)        │ [蓝色标签/红色数值]
│ 第8行: 湿度 (Hum: 65%)          │ [蓝色标签/红色数值]
└─────────────────────────────────┘
```

### 显示优化

- **背景颜色**: 黑色（0x0000）
- **字体颜色**: 白色（时间）、蓝色（标签）、红色（数值）
- **刷新策略**: 时间显示只更新变化的部分（秒、分钟、小时、日期）
- **字体大小**: 16号字体
- **显示模式**: 80x160竖屏模式

### 天气状况中英文转换

- 晴 → SUNNY
- 多云 → CLOUDY
- 阴 → OVERCAST
- 雨 → RAIN
- 雪 → SNOW
- 雾 → FOG

### 风向中英文转换

- 北 → N
- 东北 → NE
- 东 → E
- 东南 → SE
- 南 → S
- 西南 → SW
- 西 → W
- 西北 → NW

## ⚙️ 配置说明

### sdkconfig 关键配置

```
# Flash配置
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y

# WiFi配置
CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM=10
CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM=32

# HTTP客户端配置
CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS=y
CONFIG_ESP_HTTP_CLIENT_ENABLE_GZIP=y

# 日志配置
CONFIG_LOG_DEFAULT_LEVEL_INFO=y

# FreeRTOS配置
CONFIG_FREERTOS_HZ=100
```

## ❓ 常见问题

### Q1: WiFi连接失败

**解决方法：**
```bash
# 检查配置
cat main/config.h | grep WIFI_

# 查看日志
idf.py monitor | grep "WIFI_MANAGER"

# 重置WiFi
idf.py erase_flash
idf.py flash monitor
```

### Q2: NTP同步失败

**解决方法：**
```bash
# 查看日志
idf.py monitor | grep "NTP_CLIENT"
```

### Q3: 天气数据获取失败

**解决方法：**
```bash
# 检查API密钥
cat main/config.h | grep API_KEY

# 查看日志
idf.py monitor | grep "WEATHER_CLIENT"
```

### Q4: 显示异常

**解决方法：**
```bash
# 检查引脚连接
# 查看日志
idf.py monitor | grep "LCD"
```

## 📊 性能指标

| 指标 | 数值 |
|------|------|
| **固件大小** | ~1MB |
| **RAM占用** | ~40KB |
| **WiFi连接时间** | <3秒 |
| **NTP同步时间** | <2秒 |
| **天气API响应时间** | <500ms |
| **GZIP压缩率** | ~40% |
| **LCD刷新频率** | 1Hz |
| **功耗** | ~20mA |

## 🔍 调试和日志

### 日志级别

```bash
# 在menuconfig中配置
idf.py menuconfig → Component config → Log output → Default log level
```

### 调试技巧

```bash
# 过滤特定模块的日志
idf.py monitor | grep "WEATHER_CLIENT"
idf.py monitor | grep "WIFI_MANAGER"
idf.py monitor | grep "NTP_CLIENT"

# 查看错误日志
idf.py monitor | grep "ERROR"
idf.py monitor | grep "FAIL"
```

## 📄 许可证

本项目仅供学习和参考使用。

## � 版本历史

### v1.2.0 (2026-02-14)
- ✅ 修复LCD显示问题：字体背景色块问题
- ✅ 实现黑色背景显示，字体清晰可见
- ✅ 优化时间显示刷新策略，只更新变化部分
- ✅ 添加字体外部声明，避免重复包含
- ✅ 修复系统初始化重复问题
- ✅ 修复天气数据重复获取问题
- ✅ 修复时间冒号消失问题
- ✅ 修复Temp和WX行消失问题
- ✅ 更新README文档

### v1.1.0 (2026-02-13)
- ✅ 添加更多天气信息（体感温度、风向、风力、湿度）
- ✅ 实现天气状况和风向中英文转换
- ✅ 优化代码结构，模块化重构
- ✅ 改进错误处理和内存管理
- ✅ 修复栈溢出问题

### v1.0.0 (2026-02-12)
- ✅ 初始版本发布
- ✅ 基础WiFi连接功能
- ✅ NTP时间同步
- ✅ 天气数据获取
- ✅ TFT LCD显示

## �🙏 致谢

- ESP-IDF 开发团队
- 和风天气API
- 正点原子开发板

---

**开始你的ESP32-S3物联网之旅吧！** 🚀
