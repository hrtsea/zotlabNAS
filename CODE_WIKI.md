# ZotLab NAS Monitor —— Code Wiki

> 项目版本：`APP_VERSION = "1.1.0"`  
> 目标硬件：WaveShare / TuneBar **ESP32-S3**（172×640 IPS 显示屏 + 电容触摸 + ES8311 音频 DAC + ES7210 ADC + PCF85063 RTC + TCA9554 IO 扩展）  
> 开发框架：PlatformIO + Arduino-ESP32 + LVGL v8.3.11 + SquareLine Studio  
> 许可证：MIT（根目录 `LICENSE`）

---

- [1. 项目简介](#1-项目简介)
- [2. 整体架构与数据流](#2-整体架构与数据流)
- [3. 目录结构](#3-目录结构)
- [4. 核心模块详解](#4-核心模块详解)
  - [4.1 主程序入口 `main.cpp`](#41-主程序入口-maincpp)
  - [4.2 数据模型 `data/nas_data.h` + `data/nas_config.h`](#42-数据模型-datanasdatah--datanasconfigh)
  - [4.3 配置系统 `data/config.h/.cpp`](#43-配置系统-dataconfighcpp)
  - [4.4 数据源抽象层 `net/data_source.h/.cpp`](#44-数据源抽象层-netdata_source-hcpp)
  - [4.5 NAS / 系统客户端 `net/*_client.*`](#45-nas--系统客户端-net_client)
  - [4.6 Wi-Fi 管理 `net/network.*` + `net/wifi_manager.*`](#46-wi-fi-管理-netnetwork--netwifi_manager)
  - [4.7 LVGL 移植层 `lvgl_port/`](#47-lvgl-移植层-lvgl_port)
  - [4.8 UI 层 `ui/` + `ui/screens/`](#48-ui-层-ui--uiscreens)
  - [4.9 实时任务 `xtask.cpp`](#49-实时任务-xtaskcpp)
  - [4.10 硬件驱动（BSP）](#410-硬件驱动bsp)
  - [4.11 音频子系统 `ESP32-audioI2S-master/`](#411-音频子系统-esp32-audioi2s-master)
  - [4.12 天气模块 `weather/`](#412-天气模块-weather)
  - [4.13 OTA 升级 `updater/`](#413-ota-升级-updater)
  - [4.14 文件系统 `file/`](#414-文件系统-file)
- [5. 关键类与函数索引](#5-关键类与函数索引)
- [6. 构建与烧录（PlatformIO）](#6-构建与烧录platformio)
- [7. 配置、端口与引脚总览](#7-配置端口与引脚总览)
- [8. 依赖关系与第三方库](#8-依赖关系与第三方库)
- [9. 扩展与二次开发建议](#9-扩展与二次开发建议)
- [10. 常见问题 / FAQ](#10-常见问题--faq)

---

## 1. 项目简介

**ZotLab NAS Monitor** 是一款基于 ESP32-S3 的嵌入式「NAS 监控副屏」固件。它通过 Wi-Fi 连接到主流 NAS / 服务器（Synology DSM、QNAP QTS、TrueNAS、Unraid、FNOS、Netdata、Windows、Linux 等），以统一的数据结构展示：

- CPU / 内存使用率、温度
- 磁盘状态、容量、温度、读写速度
- RAID / 卷组使用率
- 多网口实时上下行速率
- Docker / 服务运行状态
- 风扇速度与温控曲线
- 时间、天气、OTA 升级提示

通过 LVGL v8.3.11 在 172×640 长条屏上构建出一套多页面滑动 UI，配
合 SquareLine Studio 快速迭代界面；所有 NAS 客户端通过 **统一的
`DataSource` 抽象接口**连接到同一 `NasData` 数据结构，做到"UI 层只消费
一份数据模型，添加新 NAS 类型只需追加一个 client 实现"。

相关文件：
- [README.md](file:///c:/Users/steven/zotlabNAS/README.md) —— 项目总览与功能列表
- [sketch/README.md](file:///c:/Users/steven/zotlabNAS/sketch/README.md) —— PlatformIO 项目说明
- [sketch/platformio.ini](file:///c:/Users/steven/zotlabNAS/sketch/platformio.ini) —— 构建配置

---

## 2. 整体架构与数据流

```
                ┌──────────────────────────────────────────┐
                │           LVGL UI (Core 0 / DMA)          │
                │  ui.c ─ ui_helpers.c ─ ui_Screen_*.c     │
                │          ↕ lvgl_port_lock()             │
                └───────────────▲──────────────────────────┘
                                │  (只读) NasData
                                │
  ┌─────────────────────────────┴──────────────────────────────┐
  │                    NasData (统一数据模型)                 │
  │   system / disks[] / volumes[] / services[] / interfaces[] │
  │   fan / last_update_ms / is_online / has_update            │
  └───────────────▲────────────────────────────▲───────────────┘
                  │  (poll)                     │
   ┌──────────────┴──────────────┐    ┌────────┴──────────┐
   │     DataSource (抽象基类)    │    │   Preferences (NVS) │
   │  init / connect / poll /    │    │  g_config: WiFi,   │
   │  isConnected / getData()    │    │   NAS type, ports,  │
   └─────────┬────┬─────────────┘    │    poll interval    │
             │    │     ...           └────────────────────┘
       ┌─────┘    └──────┐
       ▼                 ▼
  SynologyClient   QnapClient  TrueNASClient  FnosDataSource
  UnraidDataSource NetdataClient SnmpClient
  ApiClient (Linux/HTTP + Windows)  SerialClient  MockDataSource
       └────────────────┬──────────────────────────┘
                        ▼
                HTTPClient / WiFiClientSecure
                        │
                        ▼
                 真实的 NAS / 服务器
```

### FreeRTOS 任务分配（均固定在 Core 1，Core 0 留给 Wi-Fi / Bluetooth）

| Task | 栈 | 优先级 | 周期 / 触发 | 功能 |
|------|-----|--------|------------|------|
| `audio_loop_task` | 8KB | 4 | 消息队列触发 | 接收播放指令 → 调用 `audio.loop()` |
| `rtc_read_task`   | 3KB | 3 | 周期性 | 从 PCF85063 读取实时时间，更新 `now` |
| `button_input_task` | 4KB | 2 | 10ms 轮询 | BOOT / 电源按键去抖 |
| `batt_level_read_task` | 4KB | 1 | 周期性 | 电池 ADC 电压采样 |
| `cpu_monitor_task` | 3KB | 1 | 周期性 | 统计 ESP32 自身负载（UI 调试用） |
| `fan_control_task` | 4KB | 2 | 周期性 | PWM 风扇温控曲线 + 堵转检测 |
| `lvgl_task`（内部） | — | — | `lv_timer_handler()` | 由 `lvgl_port.cpp` 启动，刷新 UI |

---

## 3. 目录结构

```
zotlabNAS/
├── LICENSE
├── README.md                    # 项目总览（中文）
├── TuneBar.code-workspace       # VS Code 工作区
├── install-uv.ps1               # 辅助脚本（Python uv 安装）
├── lightmate.html               # 项目介绍 / 早期概念（HTML）
├── document/                    # 硬件文档
│   ├── ESP32-S3-Touch-LCD-3.49-Schematic.pdf
│   ├── showcase.jpg
│   └── weather.jpg
│
└── sketch/                      # ⭐ PlatformIO 主项目目录
    ├── platformio.ini           # 构建 + 依赖配置
    ├── boards/
    │   └── esp32-s3-devkitc1-n16r8.json   # 自定义板定义
    ├── data/                    # LittleFS / SD 数据
    │   ├── stations.csv         # 天气站点列表
    │   └── audio/               # 提示音资源（ding / on / off ...）
    ├── include/
    │   ├── lv_conf.h            # LVGL v8.3.11 配置（字体/组件开关）
    │   ├── user_config.h        # 📌 硬件引脚 + 应用默认值
    │   ├── fan_pwm_bsp.h        # 风扇 PWM BSP
    │   └── xtask.h              # 任务函数声明
    ├── lib/
    │   └── ArduinoJson/         # 本地 Vendored ArduinoJson v7（示例、测试完整）
    └── src/
        ├── main.cpp             # setup() / loop() 入口
        ├── xtask.cpp            # FreeRTOS 硬件任务实现
        ├── fan_pwm_bsp.c        # 风扇 PWM BSP 实现
        │
        ├── data/                # 📦 数据模型 + 配置
        │   ├── nas_config.h     # 默认端口、数组上限、屏幕参数
        │   ├── nas_data.h       # NasSystemInfo / NasDiskInfo / NasData ...
        │   ├── config.h         # NVS 配置结构体 AppConfig
        │   └── config.cpp       # 读写 NVS、备份 / 恢复
        │
        ├── net/                 # 🌐 网络与数据源
        │   ├── data_source.h    # DataSource 抽象 + 工厂函数
        │   ├── data_source.cpp  # 工厂 + NAS_TYPES[] 配置
        │   ├── api_client.h     # ApiClient（通用 HTTP/JSON）
        │   ├── api_client.cpp
        │   ├── synology_client.h|cpp    # Synology DSM WebAPI
        │   ├── qnap_client.h|cpp         # QNAP QTS
        │   ├── truenas_client.h|cpp      # TrueNAS (SCALE/CORE)
        │   ├── fnos_client.h             # 飞牛 FNOS
        │   ├── unraid_client.h           # Unraid
        │   ├── netdata_client.h|cpp      # Netdata REST
        │   ├── snmp_client.h|cpp         # SNMP v1/v2c/v3
        │   ├── serial_client.h|cpp       # 串口协议（Linux）
        │   ├── mock_client.h|cpp         # Mock（开发 + 演示）
        │   ├── network.h|cpp             # Wi-Fi 连接、HTTP 助手、NTP/OTA
        │   └── wifi_manager.h|cpp        # （更高级的 Wi-Fi 管理器）
        │
        ├── ui/                  # 🖼️ LVGL UI（SquareLine Studio 导出）
        │   ├── ui.h / ui.c              # 主 UI 初始化
        │   ├── ui_events.h|cpp          # UI 事件回调（按键、页面切换）
        │   ├── ui_helpers.h|.c          # 通用绘图 / 文字辅助函数
        │   ├── images/                   # PNG 转成的 C 数组（时区/按钮/地球）
        │   ├── fonts/                    # 字体（NotoSansThai16）
        │   └── screens/
        │       ├── ui_Screen_Boot.h|.c       # 启动画面
        │       ├── ui_Screen_Overview.h|.cpp # 总览页（CPU/内存/网络）
        │       ├── ui_Screen_Storage.h|.c    # 存储页（磁盘阵列）
        │       └── ui_Screen_Settings.h|.c   # 设置页（WiFi/NAS/显示/风扇）
        │
        ├── lvgl_port/
        │   ├── lvgl_port.h           # lvgl_port_init / lock / unlock
        │   └── lvgl_port.cpp         # 显示驱动 + 触摸 + LVGL tick + UI 任务
        │
        ├── touch/
        │   └── esp_lcd_touch.h|.c    # 电容触摸 I2C 驱动
        │
        ├── axs15231b/
        │   └── esp_lcd_axs15231b.h|.c   # AXS15231B QSPI LCD 驱动
        │
        ├── i2c_bsp/
        │   └── i2c_bsp.h|.c         # I2C 总线初始化 (ESP-IDF driver)
        │
        ├── lcd_bl_bsp/
        │   └── lcd_bl_pwm_bsp.h|.c  # LCD 背光 PWM 控制
        │
        ├── pcf85063/
        │   └── pcf85063.h|.cpp      # PCF85063 RTC（I2C）
        │
        ├── tca9554/
        │   └── tca9554.h|.cpp       # TCA9554 8-bit IO 扩展器
        │
        ├── es8311/
        │   └── es8311.h|.cpp        # ES8311 低功耗音频 DAC
        │
        ├── es7210/
        │   ├── es7210.h|.cpp        # ES7210 ADC（MIC）
        │   └── es7210_reg.h         # ES7210 寄存器定义
        │
        ├── ESP32-audioI2S-master/   # 🔊 音频播放库（本地 Vendor）
        │   ├── Audio.h|.cpp          # 主类
        │   ├── wav_decoder/          # WAV
        │   ├── mp3_decoder/          # MP3
        │   ├── vorbis_decoder/       # OGG Vorbis
        │   ├── opus_decoder/         # Opus
        │   ├── flac_decoder/         # FLAC
        │   └── aac_decoder/          # AAC (libfaad)
        │
        ├── file/
        │   └── file.h|.cpp          # LittleFS 初始化 + 工具函数
        │
        ├── weather/
        │   └── weather.h|.cpp       # 天气 API 客户端
        │
        ├── updater/
        │   └── updater.h|.cpp       # OTA (HTTP) 升级
        │
        └── task_msg/
            └── task_msg.h|.cpp       # 任务间消息 / 日志辅助
```

---

## 4. 核心模块详解

### 4.1 主程序入口 `main.cpp`

- 文件：[sketch/src/main.cpp](file:///c:/Users/steven/zotlabNAS/sketch/src/main.cpp)

`setup()` 执行流程（严格顺序）：

1. `Serial.begin(115200)` —— 调试串口
2. `i2c_master_Init()` —— 初始化两路 I2C（主 I2C SDA=GPIO47/SCL=GPIO48；触摸 I2C SDA=GPIO17/SCL=GPIO18）
3. `new TCA9554(...)` + `io->begin()` —— IO 扩展器（控制电源保持 / 功放使能）
4. `lvgl_port_init()` —— 启动 LVGL（显示、触摸、tick、UI 任务、线程锁）
5. `ui_init()` —— 通过 LVGL 定时器延后初始化 UI，避免在 `setup()` 阻塞
6. `initLittleFS()` —— 挂载 LittleFS（音频文件、WiFi 凭据 JSON 等）
7. `lcd_bl_pwm_bsp_init()` —— LCD 背光 PWM（GPIO8）
8. 功放使能 `EXIO7=1`，随后初始化 ES8311（DAC）与 ES7210（MIC）
9. `Audio` 实例设置 I2S 引脚 + mono + timeout，并创建 `audio_cmd_queue`（队列深度 10）
10. `xTaskCreatePinnedToCore(...)` —— 批量创建硬件任务（见 §2 表格）
11. `config_load()` —— 从 NVS 读取 `AppConfig`
12. 如果存在 WiFi SSID，则 `wifiEnable=true; wifiConnect();`

`loop()` 执行流程：

1. 检测 `g_config.nas_type` 是否发生变化 —— 若变化则调用 `switchDataSource()` 重建数据源实例
2. `vTaskDelay(100ms)` —— 主循环不做繁重工作，主要为了"配置变更检测"

> 关键点：**NAS 数据源由 `main.cpp` 的 loop 周期监测配置变化而热切换**，避免
> 在事件回调中销毁对象导致的死锁 / 崩溃。UI 只通过 `g_data_source->getData()`
> 读取统一模型。

---

### 4.2 数据模型 `data/nas_data.h` + `data/nas_config.h`

- [nas_data.h](file:///c:/Users/steven/zotlabNAS/sketch/src/data/nas_data.h)
- [nas_config.h](file:///c:/Users/steven/zotlabNAS/sketch/src/data/nas_config.h)

#### 4.2.1 `NasType` 枚举

```cpp
enum NasType {
  NAS_SYNOLOGY, NAS_QNAP, NAS_TRUENAS, NAS_FNOS, NAS_UNRAID,
  NET_LINUX_HTTP, NET_LINUX_SERIAL, NET_WINDOWS,
  NET_NETDATA, NET_SNMP,
  NAS_MOCK,
  NAS_TYPE_ENUM_COUNT
};
```

字符串 ID 与枚举值的互转由 `nasTypeFromString()` / `nasTypeToString()` 在
`data_source.cpp` 中实现。

#### 4.2.2 顶层 `NasData`

```cpp
struct NasData {
  NasSystemInfo   system;                      // 主机信息
  NasDiskInfo     disks[MAX_DISKS];            // 磁盘数组（默认上限 16）
  uint8_t         disk_count;
  NasVolumeInfo   volumes[MAX_VOLUMES];        // RAID / 卷
  uint8_t         volume_count;
  NasServiceInfo  services[MAX_SERVICES];      // Docker/服务
  uint8_t         service_count;
  NasNetworkInfo  network;                     // 兼容旧字段（当前活动网口）
  NasInterfaceInfo interfaces[MAX_NETWORK_INTERFACES];  // 多网卡
  uint8_t         interface_count;
  uint8_t         active_interface_idx;
  FanStatus       fan;
  uint32_t        last_update_ms;
  bool            is_online;
  bool            has_update;
};
```

核心约束：

| 宏 | 默认值 | 含义 |
|----|--------|------|
| `MAX_DISKS` | 16 | 磁盘数组上限 |
| `MAX_VOLUMES` | 8 | 卷组上限 |
| `MAX_SERVICES` | 16 | 服务上限 |
| `MAX_CPU_CORES` | 8 | CPU 核心数组上限 |
| `MAX_NETWORK_INTERFACES` | 4 | 网口上限 |
| `NET_HISTORY_POINTS` | 30 | 网络速率历史长度（供趋势图使用） |
| `DEFAULT_POLL_SEC` | 5 | 轮询间隔（秒） |
| `TOTAL_PAGES` | 7 | UI 页面总数 |

#### 4.2.3 子结构

- **`NasSystemInfo`**：hostname、model、uptime_s、cpu_pct、ram_pct、ram_total/used/free/cached_mb、temp_cpu、temp_sys、cpu_cores[8]、load_avg[3]
- **`NasDiskInfo`**：name、device、model_name、mount、disk_type（SSD/HDD）、temp、health、size_gb、used_gb、used_pct、read_kbps、write_kbps
- **`NasVolumeInfo`**：name、total_gb、used_gb、used_pct、raid、status
- **`NasServiceInfo`**：name、running、is_docker
- **`NasInterfaceInfo`**：name、ip、rx_bps、tx_bps、active
- **`FanStatus`**：rpm、pwm_pct、ctrl_temp、stall_alarm、enabled
- **`FanConfig` / `FanCurvePoint[FAN_CURVE_POINTS]`**：完整温控曲线 + 模式 + 滞回 + ramp + stall_detect（由 `config.h` 的 `AppConfig.fan` 承载）

---

### 4.3 配置系统 `data/config.h/.cpp`

- [config.h](file:///c:/Users/steven/zotlabNAS/sketch/src/data/config.h)
- [config.cpp](file:///c:/Users/steven/zotlabNAS/sketch/src/data/config.cpp)

项目使用 ESP32 **NVS（Non-Volatile Storage）** 保存所有可调配置。命名空间为
`"nasmon"`，所有字段通过 `Preferences` 库以 key-value 形式读写。

#### 4.3.1 `AppConfig`

包含以下配置域（每个域都有对应的 NVS key 宏，方便 grep）：

| 域 | 字段 |
|----|------|
| Wi-Fi | `ssid[33]`、`wifipass[64]` |
| NAS | `nas_type[16]`、`nas_ip[40]`、`nas_port`、`nas_user[32]`、`nas_pass[65]`、`nas_https`、`snmp_comm[32]`、`snmp_ver`、`serial_baud` |
| UI / 轮询 | `poll_sec`、`rotation_angle`、`brightness`、`autodim`、`timezone`、`sata_disk_count`、`m2_disk_count` |
| 自动循环 | `auto_cycle_enabled`、`auto_cycle_interval_sec` |
| 天气 | `weather_api_key[65]`、`weather_city[32]` |
| 风扇 | `FanConfig fan`（见 `nas_data.h`） |

#### 4.3.2 主要 API

| 函数 | 作用 |
|------|------|
| `void config_load()` | 从 NVS 读取所有字段，缺省用 `nas_config.h` 中 `DEFAULT_*` 宏填充 |
| `void config_save()` | 将当前 `g_config` 写回 NVS |
| `config_save_wifi(ssid, pass)` | 单独写 Wi-Fi 凭据（含输入校验 + 打印字符过滤） |
| `config_save_nas(type, ip, port, user, pass, https)` | 写 NAS 连接参数 |
| `config_save_display(rotation, brightness, autodim)` | 写显示设置 |
| `config_save_fan(FanConfig&)` | 写完整风扇配置（曲线 blob） |
| `config_save_disk_config(sata_count, m2_count)` | 写磁盘数量，超过 `MAX_DISKS` 自动按比例缩减 |
| `config_reset()` | 清空 NVS 命名空间，重新加载默认值 |
| `config_save_backup()` / `config_restore_backup()` | 在"应用新 NAS 配置失败时回退"时使用（`g_config_backup`） |

#### 4.3.3 默认值一览（位于 `nas_config.h`）

```cpp
#define DEFAULT_SATA_DISK_COUNT   6
#define DEFAULT_M2_DISK_COUNT     3
#define DEFAULT_POLL_SEC          5
#define DEFAULT_HTTP_PORT         8099   // Linux HTTP 通用端口（nasmon.py）
#define DEFAULT_SYNOLOGY_PORT    5000   // DSM HTTP（HTTPS 使用 5001）
#define DEFAULT_QNAP_PORT        8080
#define DEFAULT_TRUENAS_PORT      80
#define DEFAULT_NETDATA_PORT    19999
#define DEFAULT_SNMP_PORT        161
#define DEFAULT_SERIAL_BAUD  115200
```

---

### 4.4 数据源抽象层 `net/data_source.h/.cpp`

- [data_source.h](file:///c:/Users/steven/zotlabNAS/sketch/src/net/data_source.h)
- [data_source.cpp](file:///c:/Users/steven/zotlabNAS/sketch/src/net/data_source.cpp)

**这是整个项目最关键的"解耦设计"。** 所有 NAS/系统客户端都继承自
`DataSource`，统一的生命周期：

```cpp
class DataSource {
public:
  virtual ~DataSource() = default;
  virtual bool init(Preferences& prefs) = 0;     // 读 NVS 参数
  virtual bool connect() = 0;                     // 建立会话（登录 / 握手）
  virtual void disconnect();                      // 可选的优雅断开
  virtual bool poll() = 0;                        // 周期刷新 NasData
  virtual bool isConnected() = 0;                 // 连接健康检查
  virtual const NasData& getData() = 0;
  virtual const char* getTypeName() = 0;
  virtual const char* getConnIcon() = 0;
  virtual NasTypeConfig getConfig() = 0;          // 返回默认参数（供 UI 填充）
};
```

#### 4.4.1 工厂表

`data_source.cpp` 内部定义 `factory_table[]`，通过字符串 ID 动态创建实例：

```cpp
"linux_http"    → new ApiClient(), setType(NET_LINUX_HTTP)
"linux_serial"  → new SerialClient()
"windows"       → new ApiClient(), setType(NET_WINDOWS)
"synology"      → new SynologyClient()
"qnap"          → new QnapClient()
"truenas"       → new TrueNASClient()
"fnos"          → new FnosDataSource()
"unraid"        → new UnraidDataSource()
"netdata"       → new NetdataClient()
"snmp"          → new SnmpClient()
"mock"          → new MockDataSource()
```

#### 4.4.2 动态切换：`switchDataSource(const char* type_id)`

1. 若已有 `g_data_source`，调用 `disconnect()` 后 `delete`；
2. 若 type 为空 / `"none"`，视为关闭数据源；
3. 调用工厂创建新实例 → `init(prefs)` → `connect()`；
4. 失败时安全释放并返回 `false`。

`main.cpp` 的 `loop()` 会对比 `g_config.nas_type` 与缓存字符串，一旦变更
即触发 `switchDataSource()`，从而实现 UI 设置页热切换 NAS 类型。

#### 4.4.3 C 兼容包装

`ui.c`（由 SquareLine Studio 导出，纯 C）调用：

```c
bool data_source_is_connected(void);
const char* data_source_get_type_name(void);
float data_source_get_rx_speed_mbps(void);
float data_source_get_tx_speed_mbps(void);
```

---

### 4.5 NAS / 系统客户端 `net/*_client.*`

| 文件 | 类名 | 适配对象 | 关键实现点 |
|------|------|---------|----------|
| [api_client.h](file:///c:/Users/steven/zotlabNAS/sketch/src/net/api_client.h) / .cpp | `ApiClient` | Linux (HTTP)、Windows | HTTPClient + ArduinoJson 解析通用 JSON；按 `current_type` 区分字段名 |
| [serial_client.h](file:///c:/Users/steven/zotlabNAS/sketch/src/net/serial_client.h) / .cpp | `SerialClient` | Linux (Serial over USB) | UART2（GPIO43/44），自定义 STX/ETX 帧格式 |
| [synology_client.h](file:///c:/Users/steven/zotlabNAS/sketch/src/net/synology_client.h) / .cpp | `SynologyClient` | Synology DSM（5.x/6.x/7.x） | `auth.cgi` 获取 `sid`，`entry.cgi?api=SYNO.System...` 拉取系统/存储/网络 |
| [qnap_client.h](file:///c:/Users/steven/zotlabNAS/sketch/src/net/qnap_client.h) / .cpp | `QnapClient` | QNAP QTS | HTTP Basic Auth + `/cgi-bin/authLogin.cgi` + sysinfo |
| [truenas_client.h](file:///c:/Users/steven/zotlabNAS/sketch/src/net/truenas_client.h) / .cpp | `TrueNASClient` | TrueNAS SCALE / CORE | `/api/v2.0/system/info`, `/pool`, `/disk/temperature` |
| [unraid_client.h](file:///c:/Users/steven/zotlabNAS/sketch/src/net/unraid_client.h) | `UnraidDataSource` | Unraid | `/webGui/include/...` 或 Docker API |
| [fnos_client.h](file:///c:/Users/steven/zotlabNAS/sketch/src/net/fnos_client.h) | `FnosDataSource` | 飞牛 NAS | 私有 API |
| [netdata_client.h](file:///c:/Users/steven/zotlabNAS/sketch/src/net/netdata_client.h) / .cpp | `NetdataClient` | Netdata 监控代理 | `http://host:19999/api/v1/data?chart=...` |
| [snmp_client.h](file:///c:/Users/steven/zotlabNAS/sketch/src/net/snmp_client.h) / .cpp | `SnmpClient` | 支持 SNMP 的任何设备 | SNMP v1/v2c (community) / v3 (auth/priv)；常见 OID：hrSystem、hrStorage、ifTable、diskIo、lmSensors |
| [mock_client.h](file:///c:/Users/steven/zotlabNAS/sketch/src/net/mock_client.h) / .cpp | `MockDataSource` | 离线演示 | 根据时间生成伪随机 CPU/网络曲线，便于调试 UI |

> **开发约定**：每个 client 内部都应包含一个 `NasData data;` 字段，并在
> `poll()` 中按 `NasDiskInfo/NasVolumeInfo/...` 的标准结构填充字段。UI 不
> 会、也不应该关心数据来自哪个 NAS 品牌。

---

### 4.6 Wi-Fi 管理 `net/network.*` + `net/wifi_manager.*`

- [network.h](file:///c:/Users/steven/zotlabNAS/sketch/src/net/network.h)
- [network.cpp](file:///c:/Users/steven/zotlabNAS/sketch/src/net/network.cpp)
- [wifi_manager.h](file:///c:/Users/steven/zotlabNAS/sketch/src/net/wifi_manager.h)
- [wifi_manager.cpp](file:///c:/Users/steven/zotlabNAS/sketch/src/net/wifi_manager.cpp)

关键全局变量：

```cpp
bool wifiEnable;                  // 全局 Wi-Fi 使能开关
bool wifi_need_connect;           // 下一次 tick 请求重连
lv_timer_t* wifi_check_timer;     // LVGL 定时器：周期性检查连接状态
TaskHandle_t wifiTaskHandle;      // 可选：独立 Wi-Fi 扫描任务
WifiEntry wifiList[WIFI_MAX=10];  // 多个已知网络（SSID / pass / rssi）
```

核心功能：

- **`loadWifiList()` / `saveWifiList()`** —— 从 `/wifi.json`（LittleFS）加载/保存已知网络凭据（ArduinoJson）。
- **`addOrUpdateWifi()`** —— 合并相同 SSID 条目。
- **`scanWiFi(bool updateList)`** —— 调用 `WiFi.scanNetworks()`，用 LVGL 弹窗列出结果。
- **`wifiConnect()`** —— 使用 `g_config.ssid` / `wifipass` 连接；失败后 5 秒重试。
- **`enableTlsInPsram()`** —— **关键优化**：为 mbedTLS 注入 `psram_calloc/psram_free`，把 TLS 大对象放进外部 PSRAM，防止 HTTPS 握手时 OOM。
- **`sanitizeJson()`** —— 过滤不可打印字符，避免 ArduinoJson 解析失败。
- **`fetchUrlData(url, ssl, outBuf, outBufSize)`** —— 通用 HTTP GET 辅助，返回 `bool` 成功/失败。

---

### 4.7 LVGL 移植层 `lvgl_port/`

- [lvgl_port.h](file:///c:/Users/steven/zotlabNAS/sketch/src/lvgl_port/lvgl_port.h)
- [lvgl_port.cpp](file:///c:/Users/steven/zotlabNAS/sketch/src/lvgl_port/lvgl_port.cpp)
- [lv_conf.h](file:///c:/Users/steven/zotlabNAS/sketch/include/lv_conf.h)

`lvgl_port_init()` 做了三件事：

1. **显示驱动初始化**：挂载 AXS15231B（QSPI）LCD；在双缓冲（`DMA_BUFF_LEN = 172*64*2` byte）模式下 flush。
2. **输入驱动注册**：使用 `esp_lcd_touch.c` 的 I2C 电容触摸作为 `indev`，周期读取坐标。
3. **启动 tick + UI 任务**：用硬件定时器或 `vApplicationTickHook` 驱动 `lv_tick_inc(5ms)`，并创建一个 FreeRTOS 任务周期性调用 `lv_timer_handler()`。

`lvgl_port_lock() / lvgl_port_unlock()` 提供跨任务的 UI 访问保护（在从硬件任务更新 UI 前必须 lock）。

`lv_conf.h` 中的关键开关：

| 宏 | 值 | 含义 |
|----|----|------|
| `LV_COLOR_DEPTH` | 16 | RGB565（与 LCD 面板一致） |
| `LV_COLOR_16_SWAP` | 1 | 字节序交换（QSPI LCD 需要） |
| `LV_MEM_SIZE` | (64U * 1024U) | LVGL 内部堆（64KB） |
| `LV_MEM_CUSTOM` | 0 | 不替换 malloc（保留内置分配器） |
| `LV_USE_PERF_MONITOR` | 1 | 显示 FPS / CPU 占用调试标签 |
| `LV_USE_MEM_MONITOR` | 1 | 显示内存占用 |
| `LV_THEME_DEFAULT_DARK` | 0 | 浅色主题（需要暗色主题置 1） |
| `LV_FONT_MONTSERRAT_*` | 多种 | 开启 12/14/16/18/20/22/24/26/28/30/32 字号 |
| `LV_USE_GRID / LV_USE_FLEX` | 1 | 页面排布使用 flex + grid |

---

### 4.8 UI 层 `ui/` + `ui/screens/`

所有 UI 代码由 **SquareLine Studio** 导出后少量手改。主要文件：

| 文件 | 功能 |
|------|------|
| [ui.h](file:///c:/Users/steven/zotlabNAS/sketch/src/ui/ui.h) / [ui.c](file:///c:/Users/steven/zotlabNAS/sketch/src/ui/ui.c) | `ui_init()` 创建所有屏幕 / 对象、分配字体 / 图片资源 |
| [ui_events.h](file:///c:/Users/steven/zotlabNAS/sketch/src/ui/ui_events.h) / [ui_events.cpp](file:///c:/Users/steven/zotlabNAS/sketch/src/ui/ui_events.cpp) | 按钮、滑动、下拉、TabView 的 LV_EVENT 回调（例如切换 NAS 类型后写 NVS） |
| [ui_helpers.h](file:///c:/Users/steven/zotlabNAS/sketch/src/ui/ui_helpers.h) / [ui_helpers.c](file:///c:/Users/steven/zotlabNAS/sketch/src/ui/ui_helpers.c) | 通用绘图函数（画仪表、画进度条、格式化数字） |
| `screens/ui_Screen_Boot.*` | 启动画面（LOGO + 版本号） |
| `screens/ui_Screen_Overview.*` | 总览页（CPU 仪表 / 内存条 / 网口上下行流量 / 时钟 / 天气 / OTA 提示） |
| `screens/ui_Screen_Storage.*` | 存储页（双列磁盘槽 + RAID 状态 + 每盘温度/使用率） |
| `screens/ui_Screen_Settings.*` | 设置页（TabView: NAS / Wi-Fi / 显示 / 风扇 / 时区 / 固件升级） |
| `images/ui_img_*.c` | 时区图标 / 地球 / 按钮等（PNG 转换的 C 数组） |
| `fonts/ui_font_NotoSanThai16.c` | 自定义字体（支持泰语等复杂文字） |

**UI → 数据的绑定方式**：`ui_events.cpp` 中对各种输入控件（例如 NAS 类型下拉
框、IP 输入框）写入 `g_config`；**数据 → UI** 则由 `Overview.cpp` 的 LVGL
定时器（每 1s 或 poll_interval）读取 `g_data_source->getData()` 并用
`lv_label_set_text_fmt()` / `lv_bar_set_value()` 刷新。

> SquareLine Studio 导出提示：导出后需要手动注释掉 `ui.c` 中与
> `PropertyAnimation_*_user_data->imgset` 相关的几行，避免编译错误。详细流程
> 见 `sketch/README.md`。

---

### 4.9 实时任务 `xtask.cpp`

- [xtask.cpp](file:///c:/Users/steven/zotlabNAS/sketch/src/xtask.cpp)
- 声明：[include/xtask.h](file:///c:/Users/steven/zotlabNAS/sketch/include/xtask.h)

所有硬件任务的实现集中在这一个文件中，便于调整任务优先级、栈大小和 Core 亲和性。
主要函数（每一个都是 `void task(void *param)` 风格的 FreeRTOS 入口）：

- **`button_input_task()`** —— 50ms 软件去抖，识别 BOOT / 电源键；长按可以触发关机（`EXIO6` 电源保持释放）。
- **`rtc_read_task()`** —— 定期读取 PCF85063，写入 `now`（由 `nas_data.h` 中 `extern RTC_DateTime now;` 共享）。
- **`batt_level_read_task()`** —— ADC1 通道采样电池电压。
- **`cpu_monitor_task()`** —— 统计 ESP32 自身的 idle 时间，用作性能调试。
- **`fan_control_task()`** —— 读取 `g_config.fan` 的曲线（温度 ↔ PWM 百分比），带滞回、ramp（软启动）、堵转检测（stall），并把 `FanStatus` 写回 `g_data_source` 的数据模型供 UI 显示。

音频任务由 `audio_loop_task` 处理，通过队列 `audio_cmd_queue`（`sizeof(int)`）接收播放指令，然后循环调用 `audio.loop()` 以驱动 I2S 数据流。

---

### 4.10 硬件驱动（BSP）

#### 4.10.1 I2C 总线 `i2c_bsp/`

- 头文件：[i2c_bsp.h](file:///c:/Users/steven/zotlabNAS/sketch/src/i2c_bsp/i2c_bsp.h)
- 实现：[i2c_bsp.c](file:///c:/Users/steven/zotlabNAS/sketch/src/i2c_bsp/i2c_bsp.c)

提供 `i2c_master_Init()` —— 用 ESP-IDF `i2c_driver_install()` +
`i2c_new_master_bus()` 初始化两条 I2C 总线：

- **主 I2C（GPIO47 SDA / GPIO48 SCL）**：挂 ES8311 (0x18)、ES7210 (0x40)、TCA9554 (0x20)、PCF85063 (0x51)、IMU QMI8658 (0x6b)
- **触摸 I2C（GPIO17 SDA / GPIO18 SCL）**：挂电容触摸 IC（地址 `0x3b`）

在 `user_config.h` 中有定义：`ESP_SDA_NUM / ESP_SCL_NUM / Touch_SDA_NUM / Touch_SCL_NUM`。

#### 4.10.2 LCD + 背光 `axs15231b/` + `lcd_bl_bsp/`

- LCD：[esp_lcd_axs15231b.h](file:///c:/Users/steven/zotlabNAS/sketch/src/axs15231b/esp_lcd_axs15231b.h) / [.c](file:///c:/Users/steven/zotlabNAS/sketch/src/axs15231b/esp_lcd_axs15231b.c)
- 背光：[lcd_bl_pwm_bsp.h](file:///c:/Users/steven/zotlabNAS/sketch/src/lcd_bl_bsp/lcd_bl_pwm_bsp.h) / [.c](file:///c:/Users/steven/zotlabNAS/sketch/src/lcd_bl_bsp/lcd_bl_pwm_bsp.c)

- **AXS15231B**：使用 SPI3_HOST（QSPI，GPIO9 CS / GPIO10 PCLK / GPIO11~14 D0~D3，RST=GPIO21）。分辨率 172×640（竖屏显示），颜色深度 RGB565。驱动实现了 `esp_lcd_panel_io_handle_t` + `esp_lcd_panel_handle_t`，`flush_cb` 通过 `lvgl_port.cpp` 接入 LVGL。
- **背光**：GPIO8 用 ESP32 LEDC PWM 驱动（`LCD_PWM_MODE_255` 表示 0~255 范围）。`lcd_bl_pwm_bsp_init(mode)` / `lcd_bl_pwm_bsp_set(level)` 为外部 API。

#### 4.10.3 RTC `pcf85063/`

- [pcf85063.h](file:///c:/Users/steven/zotlabNAS/sketch/src/pcf85063/pcf85063.h) / [.cpp](file:///c:/Users/steven/zotlabNAS/sketch/src/pcf85063/pcf85063.cpp)
- I2C 地址：`0x51`
- 提供：`setTime / getTime / setDate / getDate`，及 C 风格 `RTC_DateTime now`。
- 特点：在断电期间由备用电池供电，保持时钟；上电后由 `rtc_read_task` 读取一次并刷新 UI。

#### 4.10.4 IO 扩展器 `tca9554/`

- [tca9554.h](file:///c:/Users/steven/zotlabNAS/sketch/src/tca9554/tca9554.h) / [.cpp](file:///c:/Users/steven/zotlabNAS/sketch/src/tca9554/tca9554.cpp)
- I2C 地址：`0x20`
- **关键用途**：
  - `EXIO6`（IO6）= 电源保持。按下电源按键后，固件将此引脚置 1，使 PMIC 持续供电；关机时拉低。
  - `EXIO7`（IO7）= 音频功放使能。`0=静音，1=允许播放`。
- API：`begin() / digitalWrite(bit, v) / digitalRead(bit) / setPinMode(bit, mode)`。

#### 4.10.5 音频 DAC `es8311/`

- [es8311.h](file:///c:/Users/steven/zotlabNAS/sketch/src/es8311/es8311.h) / [.cpp](file:///c:/Users/steven/zotlabNAS/sketch/src/es8311/es8311.cpp)
- I2C 地址：`0x18`
- `speaker.setVolume(0-100)`、`speaker.begin()` 初始化 I2S 通路（I2S_DSOUT=GPIO45，I2S_BCLK=GPIO15，I2S_MCLK=GPIO7，I2S_LRC=GPIO46）。配合 `ESP32-audioI2S-master` 的 `Audio` 对象播放 MP3/WAV/...

#### 4.10.6 音频 ADC（MIC） `es7210/`

- [es7210.h](file:///c:/Users/steven/zotlabNAS/sketch/src/es7210/es7210.h) / [.cpp](file:///c:/Users/steven/zotlabNAS/sketch/src/es7210/es7210.cpp) + [es7210_reg.h](file:///c:/Users/steven/zotlabNAS/sketch/src/es7210/es7210_reg.h)
- I2C 地址：`0x40`
- 录音 / 语音唤醒相关；当前固件主要把它作为"可扩展的输入通道"。

#### 4.10.7 触摸 `touch/esp_lcd_touch.*`

- [esp_lcd_touch.h](file:///c:/Users/steven/zotlabNAS/sketch/src/touch/esp_lcd_touch.h) / [.c](file:///c:/Users/steven/zotlabNAS/sketch/src/touch/esp_lcd_touch.c)
- 使用独立 I2C 总线（GPIO17/18），周期性读取 x/y/触点数并转发给 LVGL indev。

#### 4.10.8 风扇 `fan_pwm_bsp.*`（include + src 两层）

- 头文件：[include/fan_pwm_bsp.h](file:///c:/Users/steven/zotlabNAS/sketch/include/fan_pwm_bsp.h)
- 实现：[src/fan_pwm_bsp.c](file:///c:/Users/steven/zotlabNAS/sketch/src/fan_pwm_bsp.c)
- PWM 输出：`FAN_PWM_PIN = GPIO42`；若需要堵转检测（tacho）可设置 `FAN_TACH_PIN`（默认 `GPIO_NUM_NC`）。
- 上层控制由 `xtask.cpp::fan_control_task()` 负责，按 `g_config.fan.curve[5]` 查表 + 线性插值得到目标 PWM。

---

### 4.11 音频子系统 `ESP32-audioI2S-master/`

本地 Vendored 的 `ESP32-audioI2S` 是功能完整的音频播放库（由 `schreibfaul1`
维护），本项目使用其播放本地 MP3/WAV 作为告警/按键音效。

核心文件：
- `Audio.h` / `Audio.cpp`：主类 `Audio`
  - `setPinout(bclk, lrc, dout, mclk, din)`
  - `connecttoFS(SPIFFS/LittleFS, filename)` —— 本地文件
  - `connecttoscheme(host, port, path)` —— HTTP(S)
  - `audio_info_callback` —— 播放事件回调（错误/EOF/元数据）
- 解码器：
  - `wav_decoder/`：WAV PCM
  - `mp3_decoder/`：MP3 (Helix-style)
  - `vorbis_decoder/`：Ogg Vorbis
  - `opus_decoder/`：Opus（含 `silk` + `celt` 子模块）
  - `flac_decoder/`：FLAC
  - `aac_decoder/`：AAC / HE-AAC（`libfaad/`）

> 使用方式（在 `main.cpp` / `xtask.cpp` 中）：`audio.connecttoFS(LittleFS, "/audio/ding.mp3")`
> 在 `audio_loop_task` 中循环 `audio.loop()`；播放指令由 `xQueueSend(audio_cmd_queue, &cmd, 0)` 传递。

---

### 4.12 天气模块 `weather/`

- [weather.h](file:///c:/Users/steven/zotlabNAS/sketch/src/weather/weather.h) / [.cpp](file:///c:/Users/steven/zotlabNAS/sketch/src/weather/weather.cpp)
- 通过 `g_config.weather_api_key` + `weather_city` 调用公开天气 API（HTTP），把温度/湿度/图标等信息填入数据模型并由 UI 页面展示。
- 资源文件：`document/weather.jpg`。

---

### 4.13 OTA 升级 `updater/`

- [updater.h](file:///c:/Users/steven/zotlabNAS/sketch/src/updater/updater.h) / [.cpp](file:///c:/Users/steven/zotlabNAS/sketch/src/updater/updater.cpp)
- 使用 `HTTPClient` 拉取固件镜像 → `Update.write()` → `Update.end()` → `ESP.restart()`。
- 设置页面（`ui_Screen_Settings`）会在"固件升级"Tab 中触发此流程。
- 分区配置见 `partitions/7MB_app_ota_2MB_littlefs.csv`（在 boards 目录中），给 OTA0/OTA1 各分配 7MB，LittleFS 2MB。

---

### 4.14 文件系统 `file/`

- [file.h](file:///c:/Users/steven/zotlabNAS/sketch/src/file/file.h) / [.cpp](file:///c:/Users/steven/zotlabNAS/sketch/src/file/file.cpp)
- `initLittleFS()`：`LittleFS.begin(true)`（首次格式化）。
- 提供文件枚举 / 大小计算等辅助函数，供设置页展示可用空间、管理音频资源。
- 音频文件路径约定：`/audio/*.mp3`（例如 `on.mp3`, `off.mp3`, `ding.mp3`, `ai_not_support.mp3`, `error.mp3`）。
- 天气站点列表：`/stations.csv`（见 `data/` 目录）。

---

## 5. 关键类与函数索引

下表按"功能域"列出核心符号与实现位置，便于二次开发时 grep / 跳转。

### 5.1 入口与循环

| 符号 | 文件 | 作用 |
|------|------|------|
| `setup()` | `src/main.cpp:84` | 启动顺序总入口 |
| `loop()` | `src/main.cpp:202` | NAS 类型变更检测 + `switchDataSource()` |
| `g_data_source` | `src/net/data_source.cpp:135` | 全局当前数据源指针 |
| `g_config` | `src/data/config.cpp:4` | 全局 NVS 配置对象 |

### 5.2 DataSource 抽象与工厂

| 符号 | 文件 | 作用 |
|------|------|------|
| `class DataSource` | `src/net/data_source.h:24` | 纯虚基类 |
| `switchDataSource(type)` | `src/net/data_source.cpp:167` | 销毁旧实例 → 创建新实例 → init → connect |
| `createDataSource(type_id)` | `src/net/data_source.cpp:81` | 查表构造具体 client |
| `NAS_TYPES[]` | `src/net/data_source.h:48` | 字符串 ID ↔ 显示名 ↔ 枚举值 |
| `NasTypeConfig::getDefaults()` | `src/net/data_source.h:20` | 获取某 NAS 类型的默认 IP/端口/用户名等 |

### 5.3 配置管理

| 符号 | 文件 |
|------|------|
| `struct AppConfig` | `src/data/config.h:49` |
| `config_load() / config_save()` | `src/data/config.cpp:9/101` |
| `config_save_wifi() / config_save_nas()` | `src/data/config.cpp:147/174` |
| `config_save_fan()` | `src/data/config.cpp:236` |
| `config_save_disk_config()` | `src/data/config.cpp:253` |
| `config_save_backup() / config_restore_backup()` | `src/data/config.cpp:279/294` |
| `NVS_NAMESPACE = "nasmon"` | `src/data/config.h` | NVS 命名空间宏 |

### 5.4 数据模型

| 符号 | 文件 |
|------|------|
| `struct NasSystemInfo` | `src/data/nas_data.h:48` |
| `struct NasDiskInfo` | `src/data/nas_data.h:67` |
| `struct NasVolumeInfo` | `src/data/nas_data.h:84` |
| `struct NasServiceInfo` | `src/data/nas_data.h:93` |
| `struct NasInterfaceInfo` | `src/data/nas_data.h:108` |
| `struct FanStatus` | `src/data/nas_data.h:116` |
| `struct NasData` | `src/data/nas_data.h:124` |
| `struct FanCurvePoint / FanConfig` | `src/data/nas_data.h:143/148` |
| `MAX_DISKS / MAX_VOLUMES / MAX_SERVICES / MAX_NETWORK_INTERFACES` | `src/data/nas_config.h` |

### 5.5 硬件任务

| 函数 | 文件 | 功能 |
|------|------|------|
| `button_input_task()` | `src/xtask.cpp:27` | BOOT/电源按键 |
| `rtc_read_task()` | `src/xtask.cpp` | RTC 时间同步 |
| `batt_level_read_task()` | `src/xtask.cpp` | 电池电量 |
| `cpu_monitor_task()` | `src/xtask.cpp` | ESP32 CPU 负载 |
| `fan_control_task()` | `src/xtask.cpp` | 风扇 PWM 闭环 |
| `audio_loop_task()` | `src/xtask.cpp` | I2S 音频播放循环 |

### 5.6 驱动层

| 模块 | 文件 |
|------|------|
| I2C 总线 | `src/i2c_bsp/i2c_bsp.{h,c}` |
| LCD (AXS15231B) | `src/axs15231b/esp_lcd_axs15231b.{h,c}` |
| 背光 PWM | `src/lcd_bl_bsp/lcd_bl_pwm_bsp.{h,c}` |
| 触摸 | `src/touch/esp_lcd_touch.{h,c}` |
| RTC (PCF85063) | `src/pcf85063/pcf85063.{h,cpp}` |
| IO 扩展 (TCA9554) | `src/tca9554/tca9554.{h,cpp}` |
| 音频 DAC (ES8311) | `src/es8311/es8311.{h,cpp}` |
| 麦克风 ADC (ES7210) | `src/es7210/es7210.{h,cpp}` + `es7210_reg.h` |
| 风扇 PWM | `include/fan_pwm_bsp.h` + `src/fan_pwm_bsp.c` |
| LVGL 移植 | `src/lvgl_port/lvgl_port.{h,cpp}` |

---

## 6. 构建与烧录（PlatformIO）

项目使用 **PlatformIO**（VS Code 插件）管理编译与依赖。
相关文件：

- [sketch/platformio.ini](file:///c:/Users/steven/zotlabNAS/sketch/platformio.ini)
- [sketch/boards/esp32-s3-devkitc1-n16r8.json](file:///c:/Users/steven/zotlabNAS/sketch/boards/esp32-s3-devkitc1-n16r8.json)
- [sketch/build_and_upload.ps1](file:///c:/Users/steven/zotlabNAS/sketch/build_and_upload.ps1)（PowerShell 一键脚本）
- [sketch/compile.ps1](file:///c:/Users/steven/zotlabNAS/sketch/compile.ps1)（仅编译）

### 6.1 `platformio.ini` 关键配置

```ini
[env:esp32-s3-devkitc1-n16r8]
platform  = https://github.com/pioarduino/platform-espressif32/releases/download//55.03.35/platform-espressif32.zip
board     = esp32-s3-devkitc1-n16r8
framework = arduino
board_build.filesystem = littlefs
board_build.partitions = partitions/7MB_app_ota_2MB_littlefs.csv

lib_deps =
  lvgl/lvgl@^8.3.11
  bblanchon/ArduinoJson@^7.4.2

build_flags =
  -DCONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC=y
  -DBOARD_HAS_PSRAM
  -DCORE_DEBUG_LEVEL=3
  -DLV_CONF_INCLUDE_SIMPLE
  -DLV_LVGL_H_INCLUDE_SIMPLE
  -Isrc -Isrc/ui -Iinclude

monitor_speed  = 115200
upload_speed   = 921600
build_type     = debug
monitor_filters = esp32_exception_decoder, default
```

- **pioarduino** 分支（非官方 espressif32）：提供更新的 Arduino-ESP32 + ESP-IDF 组合。
- **CORE_DEBUG_LEVEL=3**：启用 `log_i/log_e/log_w`，串口能看到大量调试信息。
- **LV_CONF_INCLUDE_SIMPLE**：LVGL 通过 `#include "lv_conf.h"`（相对路径）加载配置；`-Iinclude` 确保能找到。
- **MBEDTLS_EXTERNAL_MEM_ALLOC=y**：配合 `enableTlsInPsram()` 将 TLS 堆放在 PSRAM。

### 6.2 常用命令

```bash
cd sketch

# 编译
pio run                       # 生产固件
pio run -e esp32-s3-devkitc1-n16r8 -v

# 上传固件
pio run -t upload

# 上传 LittleFS 文件系统（首次 / 修改 data/audio / stations.csv 后）
pio run -t uploadfs

# 监视串口输出（含异常解析）
pio device monitor -b 115200

# 清理
pio run -t clean
```

在 Windows 上可直接运行：

```powershell
cd sketch
.\compile.ps1             # 仅编译
.\build_and_upload.ps1    # 编译 + 上传固件
```

### 6.3 首次烧录流程

1. 将 ESP32-S3 通过 USB-C 连接电脑（自动出现 USB 串口）。
2. `pio run -t uploadfs` —— 把 `data/` 目录烧进 LittleFS。
3. `pio run -t upload` —— 烧写固件。
4. 打开串口监视器，首次会看到：`ESP32-S3 Hardware Template Starting... → Power ON → Power Amp OK → ES8311 OK → ES7210 OK → config_load → [WiFi] Auto-starting...`
5. 若无 SSID，可在设置页手动配置后保存，下次启动自动连接。

---

## 7. 配置、端口与引脚总览

### 7.1 引脚（来自 `include/user_config.h`）

| 功能 | GPIO | 说明 |
|------|------|------|
| LCD QSPI CS   | 9  | AXS15231B |
| LCD QSPI PCLK | 10 | |
| LCD QSPI D0~D3 | 11/12/13/14 | |
| LCD RST       | 21 | |
| LCD Backlight | 8  | PWM |
| 主 I2C SDA    | 47 | ES8311/ES7210/TCA9554/PCF85063/IMU |
| 主 I2C SCL    | 48 | |
| 触摸 I2C SDA  | 17 | |
| 触摸 I2C SCL  | 18 | |
| I2S BCLK      | 15 | |
| I2S LRC       | 46 | |
| I2S MCLK      | 7  | |
| I2S DSOUT     | 45 | → ES8311 |
| I2S DSIN      | 6  | ← ES7210 |
| SD 卡 SPI     | MOSI=39, MISO=40, SCK=41, CS=38 | 可选 |
| UART2 (串口客户端) | TX=43, RX=44 | |
| BOOT 按键     | 0  | 下拉输入 |
| SYS_OUT 电源按键 | 16 | |
| 电池 ADC      | 4  | ADC1 通道 |
| 风扇 PWM      | 42 | 可选堵转检测 TACH（默认 NC） |

### 7.2 I2C 设备地址

| 设备 | 地址 |
|------|------|
| PCF85063 (RTC) | `0x51` |
| QMI8658 (IMU)  | `0x6b` |
| ES8311 (DAC)   | `0x18` |
| TCA9554 (IO)   | `0x20` |
| ES7210 (ADC)   | `0x40` |
| 触摸 IC        | `0x3b` |

### 7.3 默认 NAS 端口

| 类型 | 协议 | 默认端口 |
|------|------|---------|
| Linux (HTTP / nasmon.py) | HTTP | 8099 |
| Synology DSM | HTTP | 5000（HTTPS 5001） |
| QNAP QTS | HTTP | 8080 |
| TrueNAS | HTTP | 80 |
| Netdata | HTTP | 19999 |
| SNMP | UDP | 161 |
| Serial | UART | 115200 bps |

可在"设置 → NAS"页面修改后写入 NVS，无需重编固件。

---

## 8. 依赖关系与第三方库

### 8.1 PlatformIO `lib_deps`（自动下载）

| 依赖 | 版本 | 用途 |
|------|------|------|
| `lvgl/lvgl` | `^8.3.11` | UI 图形库 |
| `bblanchon/ArduinoJson` | `^7.4.2` | JSON 解析 / 序列化（NAS API、WiFi 凭据文件） |

> 注：`sketch/lib/ArduinoJson/` 中还有完整的 v7 源码 + 测试 + 示例，作为本地备份和参考实现。实际编译使用的是 PlatformIO Registry 中的 `bblanchon/ArduinoJson@^7.4.2`。

### 8.2 本地 Vendored

| 路径 | 说明 |
|------|------|
| `src/ESP32-audioI2S-master/` | 完整的 I2S 音频播放库（多格式解码器） |
| `lib/ArduinoJson/` | ArduinoJson v7 源码（含 tests、examples、devcontainer CI） |

### 8.3 模块依赖图（简化版）

```
Arduino Framework (ESP32-S3)
     │
     ├── LVGL ← lv_conf.h / lvgl_port.cpp
     │          └── SquareLine Studio 导出：ui.c / ui_Screen_*.c
     │               └── ui_events.cpp (回调 → config_save_*)
     │
     ├── ArduinoJson ← config.cpp（/wifi.json）+ 所有 *client.cpp
     │
     ├── ESP32-audioI2S-master/Audio.cpp ← main.cpp / xtask.cpp
     │
     ├── HTTPClient / WiFiClientSecure ← network.cpp + api_client + 各 NAS client
     │
     ├── Preferences (NVS) ← config.cpp
     │
     └── ESP-IDF 驱动 (I2C / SPI / LEDC / UART) ← i2c_bsp.c + lcd + touch + fan...

NasData (全局数据模型)
     ↑
DataSource 抽象层
     ↑
ApiClient / SynologyClient / QnapClient / TrueNASClient / FnosDataSource
UnraidDataSource / NetdataClient / SnmpClient / SerialClient / MockDataSource
```

---

## 9. 扩展与二次开发建议

### 9.1 增加一种新的 NAS 类型

1. 在 `src/net/` 下创建 `xxx_client.h / xxx_client.cpp`；类继承 `DataSource`，内部保存 `NasData data;`。
2. 在 `net/data_source.cpp:55` 后的 `factory_table[]` 中追加一行，并在 `NAS_TYPES[]` 中追加显示名。
3. 在 `nas_config.h` 中按需追加默认端口常量（例如 `DEFAULT_XXX_PORT`），并在 `getDefaults()` 中使用。
4. 在 `ui_Screen_Settings.c` 的 NAS Tab 下拉框中增加对应选项（若 UI 是 SquareLine 生成，则回 Studio 增加选项后重新导出）。
5. 在设置页保存时写入 `g_config.nas_type = "xxx"`，重启或 `switchDataSource("xxx")` 即可生效。

### 9.2 自定义风扇温控曲线

当前曲线为 5 点（25/35/45/55/65°C → 25/30/50/80/100% PWM）。修改位置：

- `src/data/nas_data.h` 的 `DEFAULT_FAN_CURVE[]`
- 或在设置页"风扇"Tab 中手动拖曳后保存（持久化到 NVS，无需编译）

### 9.3 增加 UI 页面

1. 用 SquareLine Studio 新建一个 `ui_Screen_Xxx` 并导出到 `src/ui/screens/`。
2. 在 `main.cpp` / `ui_events.cpp` 注册页面切换事件（例如滑动手势或底部圆点）。
3. 在新页面的定时器中读取 `g_data_source->getData()` 并刷新控件。

### 9.4 增加自定义音效

1. 把 MP3/WAV 文件放到 `sketch/data/audio/`；
2. 在需要触发的地方调用：
   ```cpp
   int cmd = (int)AudioCmd::PlayDing;   // 自定义枚举
   xQueueSend(audio_cmd_queue, &cmd, 0);
   ```
3. 在 `xtask.cpp::audio_loop_task` 中解析 cmd 并调用 `audio.connecttoFS(LittleFS, "/audio/ding.mp3")`

### 9.5 开启 HTTPS / SNMP v3

- HTTPS：`g_config.nas_https = true`（Synology/TrueNAS/QNAP client 内部已支持通过 `WiFiClientSecure` 建立连接；TLS 堆放在 PSRAM）。
- SNMP v3：`g_config.snmp_ver = 3`；需同时配置 auth/priv 协议与口令（在 `SnmpClient::init()` 中解析）。

---

## 10. 常见问题 / FAQ

**Q1. 首次启动后屏幕一直黑，只打印 "ESP32-S3 Hardware Template Starting..."？**
> 检查：1) 是否 `pio run -t uploadfs` 已经执行（LittleFS 未挂载会引起后续初始化失败）；2) LCD 背光 `EXIO7` 是否被正确拉高；3) AXS15231B 的 QSPI 焊接是否完整。

**Q2. 为什么要把 mbedTLS 放在 PSRAM？**
> ESP32-S3 内部 SRAM 约 512KB，分给 Arduino heap 后 ≈ 320KB；一个 TLS 会话需要 40KB+ 连续内存。为避免 `connect()` 期间 OOM，`enableTlsInPsram()` 让 mbedTLS 直接从 PSRAM 分配。对应 `build_flags = -DCONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC=y`。

**Q3. 如何在开发时脱离真实 NAS？**
> 设置页将 NAS 类型切换为 `Mock (测试)`。`MockDataSource::poll()` 会生成带周期波动的 CPU/网络/磁盘数据，非常适合 UI 调参 / 动画打磨。

**Q4. 为什么在 `loop()` 中检测 NAS 类型变更，而不是直接在 UI 回调里调用 `switchDataSource()`？**
> UI 事件回调运行在 `lv_timer_handler()` 线程内，持有 `lvgl_port_lock`。若在此处销毁 / 创建对象并进行网络 I/O，很容易死锁或阻塞 UI。把"真正的切换操作"移到 `loop()` 可以解耦。

**Q5. PlatformIO 报 "board_build.partitions: .../7MB_app_ota_2MB_littlefs.csv 不存在"？**
> 请确认工作区目录是 `sketch/`（VS Code 打开 `TuneBar.code-workspace` 即可），并检查 `sketch/boards/esp32-s3-devkitc1-n16r8.json` 中 partitions 路径是否正确。

**Q6. 我想把屏幕横过来（172×640 → 640×172），如何处理？**
- 修改 `user_config.h` 中的 `WAVESHARE_349_LCD_H_RES / V_RES`。
- 在 AXS15231B 的初始化命令序列中切换 MADCTL 方向（`esp_lcd_axs15231b.c`）。
- 或在 LVGL 层启用 `lv_disp_set_rotation()`，配合 `LCD_NOROT_HRES / VRES` 调整。

**Q7. 如何启用 OTA 升级？**
> 在"设置 → 固件升级"页输入固件 HTTP(S) URL，点击下载。`updater.cpp` 先校验 SHA-256（如需要），然后写入 OTA 分区，最后 `ESP.restart()`。注意：分区表需要含 `ota_0` / `ota_1` / `otadata`，本项目已在 `partitions/7MB_app_ota_2MB_littlefs.csv` 中配置。

---

> **建议下一步阅读**：先熟悉 `NasData` 结构与 `DataSource` 接口（`data/nas_data.h`、`net/data_source.h`），再选一个最简单的 `mock_client.cpp` 读一遍，最后阅读 `ui_Screen_Overview.cpp` 了解数据如何上屏。
