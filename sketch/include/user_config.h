#ifndef USER_CONFIG_H
#define USER_CONFIG_H

// ============================================================
// ESP32 Hardware Development Template - 硬件配置文件
// ============================================================
// 根据你的硬件平台修改以下配置
// 示例配置基于: Waveshare ESP32-S3-Touch-LCD-3.49
// ============================================================

// ============================================================
// 1. 通信接口配置
// ============================================================

// SPI 主机配置 (用于 LCD 等外设)
#define LCD_HOST SPI3_HOST

// I2C 配置 (用于传感器、RTC、IO 扩展器等)
#define I2C_MASTER_SCL_IO GPIO_NUM_18  // I2C SCL 引脚
#define I2C_MASTER_SDA_IO GPIO_NUM_17  // I2C SDA 引脚
#define I2C_MASTER_FREQ_HZ 400000      // I2C 频率 (100kHz / 400kHz)
#define I2C_MASTER_TIMEOUT_MS 1000      // I2C 超时时间 (ms)

// SPI 配置 (用于 SD 卡等)
#define SD_CS   38  // SD 卡片选引脚
#define SPI_MOSI 39 // SPI MOSI 引脚
#define SPI_MISO 40 // SPI MISO 引脚
#define SPI_SCK  41 // SPI SCK 引脚

// ============================================================
// 2. 显示屏配置 (如硬件支持)
// ============================================================
/*
// LCD 引脚定义示例 (根据您的硬件修改)
#define LCD_PIN_CS    GPIO_NUM_9   // 片选
#define LCD_PIN_PCLK  GPIO_NUM_10  // 像素时钟
#define LCD_PIN_DATA0 GPIO_NUM_11  // 数据线 0
#define LCD_PIN_DATA1 GPIO_NUM_12  // 数据线 1
#define LCD_PIN_DATA2 GPIO_NUM_13  // 数据线 2
#define LCD_PIN_DATA3 GPIO_NUM_14  // 数据线 3
#define LCD_PIN_RST   GPIO_NUM_21  // 复位
#define LCD_PIN_BL    GPIO_NUM_8   // 背光控制

// LCD 分辨率
#define LCD_H_RES 320  // 水平分辨率
#define LCD_V_RES 240  // 垂直分辨率
*/

// ============================================================
// 3. 触摸配置 (如硬件支持)
// ============================================================
/*
#define TOUCH_SCL_NUM GPIO_NUM_18  // 触摸 I2C SCL
#define TOUCH_SDA_NUM GPIO_NUM_17  // 触摸 I2C SDA
#define TOUCH_INT_PIN  -1           // 触摸中断引脚 (-1 表示不使用)
#define TOUCH_RST_PIN  -1           // 触摸复位引脚 (-1 表示不使用)
#define TOUCH_I2C_ADDR 0x3b         // 触摸控制器 I2C 地址
*/

// ============================================================
// 4. ADC 配置
// ============================================================
#define ADC_BATT 4  // 电池电压检测 ADC 通道 (ADC1_CHANNEL_4)

// ============================================================
// 5. GPIO 配置
// ============================================================
#define BOOT 0  // BOOT 按钮引脚
#define SYS_OUT 16  // 系统电源控制引脚

// ============================================================
// 6. I2C 设备地址 (根据您的硬件修改)
// ============================================================
#define RTC_I2C_ADDR    0x51  // PCF85063 RTC 地址
#define IO_EXP_I2C_ADDR 0x20  // TCA9554 IO 扩展器地址
// #define IMU_I2C_ADDR   0x6b  // QMI8658 IMU 地址 (如适用)
// #define CODEC_I2C_ADDR 0x18  // ES8311 音频编解码器地址 (如适用)

// ============================================================
// 7. 其他配置
// ============================================================
#define CORE_DEBUG_LEVEL 3  // 调试级别 (1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=VERBOSE)

// ============================================================
// 8. 项目信息 (可选)
// ============================================================
// const char *PROJECT_NAME = "MyProject";
// const char *PROJECT_VERSION = "1.0.0";

#endif // USER_CONFIG_H
