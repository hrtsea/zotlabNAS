# ZotLab NAS Monitor

## 项目概述

ZotLab NAS Monitor 是一个基于 ESP32-S3 的嵌入式 NAS 监控屏幕固件，可同时监控 Synology DSM、QNAP QTS、TrueNAS、Unraid 等 11 种主流 NAS 系统。

**硬件平台**: Waveshare ESP32-S3-Touch-LCD-3.49 (3.49" IPS, 172×640)  
**开发框架**: pioArduino + LVGL v8.3.11 + SquareLine Studio  
**许可证**: MIT

---

## 功能特性

### 多系统支持
- **Synology DSM** - API 获取存储、硬盘、CPU/内存、温度
- **QNAP QTS** - HTTP API 获取 RAID、硬盘信息
- **TrueNAS** - REST API 监控 ZFS 存储池
- **FNOS (飞牛 NAS)** - 国产 NAS 系统
- **Unraid** - Docker API 获取数组状态
- **Netdata** - REST API 实时监控
- **SNMP** - v1/v2c/v3 标准协议
- **Linux (HTTP/Serial)** - 通用系统监控
- **Windows** - SNMP/HTTP 获取状态
- **Mock** - 开发测试模拟数据

### 核心功能
- 实时显示：存储容量、RAID 状态、硬盘温度、风扇转速、CPU/内存
- 多网口监控：最多 4 个网络接口（eth0/eth1/wlan0）
- 多设备管理：同时监控多台 NAS，快速切换
- 触控交互：触摸屏滑动切换，物理按键控制
- 风扇控制：PWM 调速、温度曲线、滞回配置
- 音量控制：ES8311 DAC + ES7210 ADC，告警音效
- OTA 升级：网络远程更新固件
- 屏幕控制：亮度调节、睡眠定时、壁纸设置

---

## 硬件配置

| 组件 | 规格 |
|------|------|
| 主控芯片 | ESP32-S3 (Xtensa dual-core) |
| 显示屏 | WaveShare 3.49" IPS (172×640) |
| 触控 | I2C 触摸控制器 |
| 音频 | ES8311 DAC + ES7210 ADC |
| 传感器 | QMI8658 (IMU + 温湿度) |
| 存储 | SD 卡 + LittleFS |
| 网络 | Wi-Fi 2.4G/5G |
| 扩展 | TCA9554 IO 扩展器 |
| RTC | PCF85063 |

---

## 项目结构

```
zotlabNAS/
├── sketch/                      # PlatformIO 项目
│   ├── src/
│   │   ├── main.cpp           # 主程序
│   │   ├── ui/               # LVGL UI 代码
│   │   │   └── screens/      # 页面实现
│   │   │       ├── ui_Screen_Overview.c
│   │   │       ├── ui_Screen_Storage.c
│   │   │       └── ui_Screen_Settings.c
│   │   ├── net/               # NAS 客户端
│   │   │   ├── synology_client.h
│   │   │   ├── qnap_client.h
│   │   │   ├── truenas_client.h
│   │   │   ├── fnos_client.h
│   │   │   ├── unraid_client.h
│   │   │   ├── netdata_client.h
│   │   │   ├── snmp_client.h
│   │   │   └── mock_client.h
│   │   ├── data/              # 数据模型
│   │   │   ├── nas_config.h   # NAS 类型配置
│   │   │   └── nas_data.h    # 统一数据模型
│   │   ├── i2c_bsp/          # I2C 总线驱动
│   │   ├── lcd_bl_bsp/       # LCD 背光驱动
│   │   ├── pcf85063/         # RTC 驱动
│   │   ├── tca9554/           # IO 扩展器驱动
│   │   ├── es8311/            # 音频 DAC 驱动
│   │   └── es7210/            # 音频 ADC 驱动
│   ├── data/                   # 文件系统数据
│   │   └── stations.csv
│   └── platformio.ini
├── spec/                       # UI Mock 页面
│   ├── ui_mock_overview.html
│   ├── ui_mock_storage.html
│   ├── ui_mock_settings.html
│   └── ui_mock_cpu.html
├── document/                   # 文档和图片
├── zotlab-nas-monitor.html     # Web Demo (交互演示)
└── README.md
```

---

## 统一数据模型

所有 NAS 客户端填充统一数据结构，UI 仅消费此结构：

```cpp
struct NasSystemInfo {
    char hostname[64];
    char model[64];
    uint32_t uptime_sec;
    float cpu_usage;
    float cpu_temp;
    float mem_total_gb;
    float mem_used_gb;
};

struct NasDiskInfo {
    char name[32];
    char model[64];
    float temp;
    uint8_t health;       // 0-100
    uint64_t capacity_gb;
    uint64_t used_gb;
    float read_speed;
    float write_speed;
};

struct NasVolumeInfo {
    char name[32];
    char raid_type[32];
    uint64_t total_gb;
    uint64_t used_gb;
};

struct NasNetworkInfo {
    char name[16];
    char ip[16];
    float rx_bps;
    float tx_bps;
};

struct FanStatus {
    uint16_t rpm;
    uint8_t pwm;
    bool enabled;
};
```

---

## 快速开始

### 1. 安装依赖

```bash
cd sketch
pio run
```

### 2. 配置 NAS 连接

编辑 `src/data/nas_config.h`，设置 NAS 类型和连接信息：

```cpp
#define NAS_TYPE NAS_TYPE_SYNOLOGY
#define NAS_HOST "192.168.1.100"
#define NAS_PORT 5000
#define NAS_USER "admin"
#define NAS_PASS "password"
```

### 3. 上传固件

```bash
# 编译并上传
pio run --target upload

# 上传文件系统
pio run --target uploadfs

# 监视串口
pio device monitor
```

---

## Web Demo

直接打开 `zotlab-nas-monitor.html` 体验交互演示，模拟 640×172 屏幕效果。

---

## UI 预览

### 总览页面
- 圆盘仪表：CPU 使用率、温度
- 进度条：内存、磁盘使用率
- LED 指示灯：硬盘在线状态

### 存储页面
- 双列 8 盘布局：6× SATA + 2× M.2
- 使用率进度条：支持 CRIT/OFF 状态显示

### 设置页面
- Tab 标签页：NAS / WiFi / Fan / Screen / Guide
- 连接状态：Connected/HTTPS ON

### CPU 页面
- 核心监控：CPU 使用率、温度
- 负载均值：1m / 5m / 15m

---

## 技术栈

- **IDE**: Visual Studio Code + PlatformIO
- **框架**: pioArduino (Arduino for ESP32-S3)
- **UI**: LVGL v8.3.11 + SquareLine Studio
- **文件系统**: LittleFS + SD 卡
- **网络**: Wi-Fi Manager + 多协议客户端

---

## License

MIT License

---

**GitHub**: https://github.com/hrtsea/zotlabNAS
