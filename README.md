# TuneBar Hardware Development Template

## 项目概述

这是一个基于 ESP32-S3 的硬件开发模板，适用于 Waveshare ESP32-S3-Touch-LCD-3.49 开发板。

**修改日期**: 2026-05-03  
**基础版本**: TuneBar v1.1.0  
**修改内容**: 移除应用逻辑，保留硬件驱动和 UI 框架

---

## 硬件平台

- **开发板**: Waveshare ESP32-S3-Touch-LCD-3.49
  - 3.49″ IPS 电容触摸屏，172 × 640 分辨率，QSPI 显示接口
  - 参考: https://www.waveshare.com/product/arduino/boards-kits/esp32-s3/esp32-s3-touch-lcd-3.49.htm?sku=32374
- **Flash 大小**: 16 MB
  - 分区: 7MB app + 2MB LittleFS
- **PSRAM**: OPI PSRAM (已启用)
- **USB CDC on Boot**: 已启用

---

## 软件栈

- **开发环境**: PlatformIO (pioArduino)
- **ESP32 Core**: 3.3.3
- **LVGL**: 8.3.11 (已恢复)
- **显示框架**: esp32_display_panel (包含依赖)
- **音频**: ESP32 Audio I2S 3.4.4 (已恢复)
  - 参考: https://github.com/schreibfaul1/ESP32-audioI2S

---

## 项目结构

```
TuneBar-main/
├── sketch/                      # PlatformIO 项目目录
│   ├── include/                 # 头文件
│   │   ├── lv_conf.h          # LVGL 配置文件 (已恢复)
│   │   ├── user_config.h      # 硬件配置文件
│   │   └── xtask.h           # FreeRTOS 任务定义
│   ├── src/                    # 源代码
│   │   ├── main.cpp          # 主程序 (已恢复)
│   │   ├── i2c_bsp/         # I2C 总线驱动
│   │   ├── lcd_bl_bsp/      # LCD 背光驱动
│   │   ├── pcf85063/         # PCF85063 RTC 驱动
│   │   ├── tca9554/          # TCA9554 IO 扩展器驱动
│   │   ├── touch/             # 触摸驱动
│   │   ├── lvgl_port/        # LVGL 移植层 (已恢复)
│   │   ├── ui/               # LVGL UI 代码 (已恢复)
│   │   ├── ESP32-audioI2S-master/  # 音频库 (已恢复)
│   │   ├── es7210/           # ES7210 麦克风驱动 (已恢复)
│   │   ├── es8311/           # ES8311 音频编解码器驱动 (已恢复)
│   │   └── axs15231b/       # AXS15231B 显示驱动 (已恢复)
│   ├── lib/                   # 库文件
│   │   ├── lvgl/            # LVGL 库 (已恢复)
│   │   └── ArduinoJson/     # ArduinoJson 库
│   ├── platformio.ini         # PlatformIO 配置 (已恢复)
│   └── partitions/            # 分区表
├── squareline/                 # SquareLine Studio 项目 (已恢复)
└── README.md                  # 本文件
```

---

## 已移除的组件

以下应用逻辑已被移除，以简化项目作为硬件开发模板：

- ❌ `src/network/` - 网络数据获取 (NAS 监控)
- ❌ `src/weather/` - 天气获取
- ❌ `src/updater/` - OTA 更新
- ❌ `src/file/` - 文件系统操作
- ❌ `src/task_msg/` - 任务消息定义

---

## 保留的组件

### 硬件驱动
- ✅ `i2c_bsp/` - I2C 总线驱动
- ✅ `lcd_bl_bsp/` - LCD 背光驱动
- ✅ `pcf85063/` - PCF85063 RTC 驱动
- ✅ `tca9554/` - TCA9554 IO 扩展器驱动
- ✅ `touch/` - 触摸驱动

### UI 框架
- ✅ `lvgl_port/` - LVGL 移植层
- ✅ `ui/` - LVGL UI 代码
- ✅ `lib/lvgl/` - LVGL 库
- ✅ `include/lv_conf.h` - LVGL 配置

### 音频驱动
- ✅ `ESP32-audioI2S-master/` - 音频库
- ✅ `es7210/` - ES7210 麦克风驱动
- ✅ `es8311/` - ES8311 音频编解码器驱动
- ✅ `axs15231b/` - AXS15231B 显示驱动

### 配置文件
- ✅ `include/user_config.h` - 完整硬件配置
- ✅ `sketch/platformio.ini` - PlatformIO 配置
- ✅ `include/xtask.h` - FreeRTOS 任务定义

---

## 使用方法

### 1. 编译项目

```bash
cd f:/nasmonitor/TuneBar-main/sketch
pio run
```

### 2. 上传固件

```bash
cd f:/nasmonitor/TuneBar-main/sketch
pio run --target upload
```

### 3. 上传 LittleFS 文件系统

```bash
cd f:/nasmonitor/TuneBar-main/sketch
pio run --target uploadfs
```

### 4. 监视串口输出

```bash
cd f:/nasmonitor/TuneBar-main/sketch
pio device monitor
```

---

## 硬件配置

所有硬件配置都在 `include/user_config.h` 中定义：

### 引脚定义
- **LCD 引脚**: `WAVESHARE_349_PIN_NUM_LCD_*`
- **触摸引脚**: `TOUCH_*`
- **I2C 引脚**: `I2C_MASTER_*`
- **SPI 引脚**: `SPI_*`
- **I2S 引脚**: `I2S_*`
- **ADC 引脚**: `ADC_BATT`
- **GPIO**: `BOOT`, `SYS_OUT`, `LCD_BL`

### I2C 设备地址
- **RTC**: `WAVESHARE_349_RTC_ADDR` (0x51)
- **IO 扩展器**: `WAVESHARE_349_EXPANDER_ADDR` (0x20)
- **音频编解码器**: `WAVESHARE_349_CODEC_ADDR` (0x18)
- **麦克风**: `WAVESHARE_349_ADC_ADDR` (0x40)
- **IMU**: `WAVESHARE_349_IMU_ADDR` (0x6b)

---

## LVGL UI 开发

### 使用 SquareLine Studio

1. 打开 `squareline/tune_bar.spj`
2. 设计 UI
3. 导出代码到 `src/ui/`
4. 编译上传

### 手动编辑 UI

直接编辑 `src/ui/ui.c` 和 `src/ui/ui.h`

---

## 音频功能

### 播放音频

```cpp
#include "ESP32-audioI2S-master/Audio.h"

Audio audio;

void setup() {
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DSOUT, I2S_MCLK, I2S_DSIN);
  audio.connecttoFS(SD, "music.mp3");
}

void loop() {
  audio.loop();
}
```

### 录制音频

```cpp
#include "es7210/es7210.h"

ES7210 mic;

void setup() {
  mic.init();
  mic.start();
}

void loop() {
  // 读取麦克风数据
}
```

---

## Git 提交历史

```
963e6eb Restore LVGL and audio drivers
a4210b8 Restore hardware configuration
547c45f Refactor: Transform to hardware development template
06ff68b Update sketch/.gitignore: Add packages to ignore list
e7682e2 Clean up: Remove unnecessary files
183a439 Initial commit: TuneBar NAS Monitor v1.1.0
```

---

## 常见问题

### 1. 编译失败

**原因**: 缺少依赖库  
**解决**: 检查 `lib/` 目录是否完整

### 2. 上传失败 (COM 端口被占用)

**原因**: Serial Monitor 未关闭  
**解决**: 关闭 Serial Monitor，或关闭占用 COM 端口的程序

### 3. LVGL 不显示

**原因**: LCD 引脚配置错误  
**解决**: 检查 `user_config.h` 中的 LCD 引脚定义

### 4. 触摸不响应

**原因**: 触摸控制器未初始化  
**解决**: 检查 `touch/touch.h` 和 I2C 地址

---

## 作者信息

- **原始项目**: TuneBar by Va&Cob
- **修改**: 移除应用逻辑，保留硬件驱动和 UI 框架
- **日期**: 2026-05-03

---

## 许可证

参考原始项目许可证。
