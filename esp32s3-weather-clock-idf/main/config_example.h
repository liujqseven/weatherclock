// config_example.h
// 复制此文件为config.h并填写真实信息

// 和风天气API配置
#define WEATHER_API_HOST "devapi.qweatherapi.com"
#define WEATHER_API_KEY "your_api_key_here"

// WiFi网络信息
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// NTP服务器配置
#define NTP_SERVER "pool.ntp.org"
#define NTP_SERVER_BACKUP "time.nist.gov"

// 地理位置信息（用于天气API）
#define LOCATION_ID "101010100"  // 北京

// 天气更新间隔（秒）
#define WEATHER_UPDATE_INTERVAL 3600

// LCD显示配置
#define LCD_BRIGHTNESS 255

// 日志级别
#define LOG_LEVEL ESP_LOG_INFO
