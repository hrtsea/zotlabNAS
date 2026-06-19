#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include <Arduino.h>

// ============================================================================
// ESP32-S3 Hardware Configuration
// Target: Waveshare ESP32-S3-Touch-LCD-3.49 (172x640)
// Based on Official WaveShare Pinout Documentation
// ============================================================================

// ============================================================
// 1. 板级信息与显示屏分辨率
// ============================================================
#define BOARD_NAME          "ESP32-S3-Touch-LCD-3.49"
#define BOARD_HAS_PSRAM     1

// 显示屏物理分辨率 - 用于 NAS Monitor 配置兼容
#ifndef TFT_HEIGHT
#define TFT_HEIGHT          172     // Portrait mode height
#endif

#ifndef TFT_WIDTH
#define TFT_WIDTH           640     // Portrait mode width
#endif

// ============================================================
// 2. 显示屏旋转配置 (LVGL)
// ============================================================
#define USER_DISP_ROT_90    0
#define USER_DISP_ROT_NONO  0
#define Rotated USER_DISP_ROT_NONO   // 软件实现旋转

// LCD 分辨率（用于 LVGL 驱动）
#if (Rotated != USER_DISP_ROT_NONO)
#define WAVESHARE_349_LCD_H_RES 172
#define WAVESHARE_349_LCD_V_RES 640
#else
#define WAVESHARE_349_LCD_H_RES 640
#define WAVESHARE_349_LCD_V_RES 172
#endif

// 未旋转时的分辨率常量
#define LCD_NOROT_HRES     172
#define LCD_NOROT_VRES     640

// LVGL 缓冲区大小配置
#define LVGL_DMA_BUFF_LEN (LCD_NOROT_HRES * 64 * 2)
#define LVGL_SPIRAM_BUFF_LEN (WAVESHARE_349_LCD_H_RES * WAVESHARE_349_LCD_V_RES * 2)

// LVGL 任务配置
#define WAVESHARE_349_LVGL_TICK_PERIOD_MS    10
#define WAVESHARE_349_LVGL_TASK_MAX_DELAY_MS  100
#define WAVESHARE_349_LVGL_TASK_MIN_DELAY_MS  10

// ============================================================
// 3. LCD QSPI 引脚配置 (AXS15231B)
// ============================================================
#define LCD_HOST SPI3_HOST  // SPI 主机端口（预留）

// LCD QSPI 数据/控制引脚（使用 WAVESHARE_349_PIN_NUM_* 长命名保持向后兼容）
#define WAVESHARE_349_PIN_NUM_LCD_CS     (GPIO_NUM_9)
#define WAVESHARE_349_PIN_NUM_LCD_PCLK   (GPIO_NUM_10)
#define WAVESHARE_349_PIN_NUM_LCD_DATA0  (GPIO_NUM_11)
#define WAVESHARE_349_PIN_NUM_LCD_DATA1  (GPIO_NUM_12)
#define WAVESHARE_349_PIN_NUM_LCD_DATA2  (GPIO_NUM_13)
#define WAVESHARE_349_PIN_NUM_LCD_DATA3  (GPIO_NUM_14)
#define WAVESHARE_349_PIN_NUM_LCD_RST    (GPIO_NUM_21)

// LCD 背光控制 - GPIO 8 (与 LCD_BL 相同值)
#define WAVESHARE_349_PIN_NUM_BK_LIGHT   (GPIO_NUM_8)

// 触摸控制器预留引脚（当前未使用，由 I2C 总线控制）
#define WAVESHARE_349_PIN_NUM_TOUCH_RST  (-1)
#define WAVESHARE_349_PIN_NUM_TOUCH_INT  (-1)

// ============================================================
// 4. I2C 总线配置
// ============================================================
// 主 I2C 总线 (I2C_NUM_0) - 连接: TCA9554, PCF85063, ES8311, ES7210, QMI8658
#define ESP_SCL_NUM  (GPIO_NUM_48)   // 主 I2C SCL
#define ESP_SDA_NUM  (GPIO_NUM_47)   // 主 I2C SDA

// 触摸 I2C 总线 (I2C_NUM_1) - 连接: 触摸控制器
#define Touch_SCL_NUM (GPIO_NUM_18)  // 触摸 I2C SCL
#define Touch_SDA_NUM (GPIO_NUM_17)  // 触摸 I2C SDA

// ============================================================
// 5. I2C 设备地址
// ============================================================
#define WAVESHARE_349_RTC_ADDR        0x51  // PCF85063 (RTC)
#define WAVESHARE_349_IMU_ADDR        0x6b  // QMI8658 (如适用)
#define WAVESHARE_349_CODEC_ADDR      0x18  // ES8311 (Audio DAC)
#define WAVESHARE_349_EXPANDER_ADDR   0x20  // TCA9554 (IO Expander)
#define WAVESHARE_349_ADC_ADDR        0x40  // ES7210 (Audio ADC)
#define I2C_TOUCH_ADDR                0x3b  // 触摸控制器

// ============================================================
// 6. I2S 音频引脚配置
// ============================================================
// ES8311 (DAC) 和 ES7210 (ADC) 共用同一 I2S 总线
// ESP32 作为 Master，提供时钟信号
#define I2S_DSIN  6   // 数据输入 - 来自 ES7210 (ADC)
#define I2S_DSOUT 45  // 数据输出 - 到 ES8311 (DAC)
#define I2S_BCLK  15  // 位时钟
#define I2S_MCLK  7   // 主时钟
#define I2S_LRC   46  // 左右声道时钟

// ============================================================
// 7. SD 卡 SPI 引脚配置
// ============================================================
#define SD_CS  38     // SD 卡片选
#define SPI_MOSI 39   // SPI MOSI (SD 卡)
#define SPI_MISO 40   // SPI MISO (SD 卡)
#define SPI_SCK  41   // SPI SCK (SD 卡)

// ============================================================
// 8. UART2 引脚配置 (用于串口客户端)
// ============================================================
// 根据官方文档: TXD -> GPIO43, RXD -> GPIO44
#define UART2_RX_PIN        GPIO_NUM_44  // UART2 接收
#define UART2_TX_PIN        GPIO_NUM_43  // UART2 发送

// ============================================================
// 9. GPIO 控制引脚
// ============================================================
#define SYS_OUT 16  // 电源按钮检测 (输入)
#define BOOT    0    // BOOT 按钮 (输入)
#define ADC_BATT 4   // 电池电压检测 ADC 通道
#define LCD_BL  8    // LCD 背光控制 - 与 WAVESHARE_349_PIN_NUM_BK_LIGHT 相同

// ============================================================
// 10. TCA9554 IO 扩展器引脚位定义
// ============================================================
// TCA9554 是 8-bit IO 扩展器，通过 I2C 连接到主 I2C 总线
// 位定义: 0bXXXXXXXB，B=0 对应 IO0，B=7 对应 IO7
#define EXIO1_BIT 0b00000001  // IO1 - backlight (预留)
#define EXIO6_BIT 0b01000000  // IO6 - power hold (电源保持)
#define EXIO7_BIT 0b10000000  // IO7 - audio_amp (音频放大器使能)

// ============================================================
// 11. 风扇 PWM 控制配置
// ============================================================
// PWM 控制信号输出引脚（连接 4 线 PWM 风扇的 PWM 线）
// GPIO42 未被其他功能占用，可安全使用
#ifndef FAN_PWM_PIN
#define FAN_PWM_PIN           (GPIO_NUM_42)
#endif

// 风扇转速反馈输入引脚（可选，用于堵转检测）
// 如果不需要堵转检测，可以不连接此引脚
#ifndef FAN_TACH_PIN
#define FAN_TACH_PIN          (GPIO_NUM_NC)
#endif

// ============================================================
// 12. 应用默认配置 (NAS Monitor)
// ============================================================
#define DEFAULT_SATA_DISK_COUNT  6   // 默认 SATA 硬盘数
#define DEFAULT_M2_DISK_COUNT    3   // 默认 M.2 硬盘数

// ============================================================
// 13. 遗留定义保留 (用于向后兼容，新代码请使用上方统一命名)
// ============================================================
// 以下定义保留用于兼容旧代码，但不推荐在新代码中使用
// I2C - 旧命名 (当前实际使用的是 ESP_* 和 Touch_*)
// #define I2C_SDA_PIN        GPIO_NUM_41  // 旧定义，未使用
// #define I2C_SCL_PIN        GPIO_NUM_42  // 旧定义，未使用

// SPI - 旧命名 (当前实际使用的是 SD_CS / SPI_MOSI / SPI_MISO / SPI_SCK)
// #define SPI_MOSI_PIN       GPIO_NUM_47  // 旧定义，未使用
// #define SPI_MISO_PIN       GPIO_NUM_48  // 旧定义，未使用
// #define SPI_SCK_PIN        GPIO_NUM_21  // 旧定义，未使用

// 背光 - 旧命名 (当前实际使用的是 WAVESHARE_349_PIN_NUM_BK_LIGHT)
// #define BL_PIN             GPIO_NUM_45  // 旧定义，未使用

#endif // USER_CONFIG_H
