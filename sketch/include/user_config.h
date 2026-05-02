#ifndef USER_CONFIG_H
#define USER_CONFIG_H

// ============================================================
// ESP32-S3 Hardware Configuration
// Target: Waveshare ESP32-S3-Touch-LCD-3.49
// ============================================================

// ============================================================
// 1. SPI & I2C 配置
// ============================================================
#define LCD_HOST SPI3_HOST

// 触摸 I2C 引脚
#define Touch_SCL_NUM (GPIO_NUM_18)
#define Touch_SDA_NUM (GPIO_NUM_17)

// ESP I2C 引脚 (用于 IO 扩展器等)
#define ESP_SCL_NUM  (GPIO_NUM_48)
#define ESP_SDA_NUM  (GPIO_NUM_47)

// ============================================================
// 2. LCD 引脚定义 (Waveshare 3.49)
// ============================================================
#define WAVESHARE_349_PIN_NUM_LCD_CS     (GPIO_NUM_9)
#define WAVESHARE_349_PIN_NUM_LCD_PCLK   (GPIO_NUM_10)
#define WAVESHARE_349_PIN_NUM_LCD_DATA0  (GPIO_NUM_11)
#define WAVESHARE_349_PIN_NUM_LCD_DATA1  (GPIO_NUM_12)
#define WAVESHARE_349_PIN_NUM_LCD_DATA2  (GPIO_NUM_13)
#define WAVESHARE_349_PIN_NUM_LCD_DATA3  (GPIO_NUM_14)
#define WAVESHARE_349_PIN_NUM_LCD_RST    (GPIO_NUM_21)
#define WAVESHARE_349_PIN_NUM_BK_LIGHT   (GPIO_NUM_8)

// 触摸控制器配置
#define I2C_TOUCH_ADDR                    0x3b
#define WAVESHARE_349_PIN_NUM_TOUCH_RST  (-1)
#define WAVESHARE_349_PIN_NUM_TOUCH_INT  (-1)

// ============================================================
// 3. LVGL 配置 (如需要)
// ============================================================
#define WAVESHARE_349_LVGL_TICK_PERIOD_MS    5
#define WAVESHARE_349_LVGL_TASK_MAX_DELAY_MS  50
#define WAVESHARE_349_LVGL_TASK_MIN_DELAY_MS  5  // 越小越灵敏

// ============================================================
// 4. I2C 设备地址
// ============================================================
#define WAVESHARE_349_RTC_ADDR    0x51  // PCF85063
#define WAVESHARE_349_IMU_ADDR    0x6b  // QMI8658 (如适用)
#define WAVESHARE_349_CODEC_ADDR  0x18  // ES8311 (如适用)
#define WAVESHARE_349_EXPANDER_ADDR 0x20  // TCA9554
#define WAVESHARE_349_ADC_ADDR    0x40  // ES7210 (如适用)

// ============================================================
// 5. 显示屏分辨率配置
// ============================================================
#define USER_DISP_ROT_90    0
#define USER_DISP_ROT_NONO  0
#define Rotated USER_DISP_ROT_NONO   // 软件实现旋转

#if (Rotated != USER_DISP_ROT_NONO)
#define WAVESHARE_349_LCD_H_RES 172
#define WAVESHARE_349_LCD_V_RES 640
#else
#define WAVESHARE_349_LCD_H_RES 640
#define WAVESHARE_349_LCD_V_RES 172
#endif

#define LCD_NOROT_HRES     172
#define LCD_NOROT_VRES     640
#define LVGL_DMA_BUFF_LEN (LCD_NOROT_HRES * 64 * 2)
#define LVGL_SPIRAM_BUFF_LEN (WAVESHARE_349_LCD_H_RES * WAVESHARE_349_LCD_V_RES * 2)

// ============================================================
// 6. I2S 音频引脚 (如需要)
// ============================================================
/*
Audio I2S pins
ES8311 → Audio DAC / playback
ES7210 → Audio ADC / microphone
They are on the SAME physical I2S bus
There is one master (ESP32), and:
ESP32 TX → ES8311 (DAC)
ESP32 RX ← ES7210 (ADC)
*/
#define I2S_DSIN  6  // data in from (es7210)
#define I2S_DSOUT 45 // data out to (es8311)
#define I2S_BCLK  15 // SCLK
#define I2S_MCLK  7
#define I2S_LRC   46 // LRCK

// ============================================================
// 7. SD 卡引脚
// ============================================================
#define SD_CS  38
#define SPI_MOSI 39
#define SPI_MISO 40
#define SPI_SCK  41

// ============================================================
// 8. ADC 配置
// ============================================================
#define ADC_BATT 4  // 电池电压检测 ADC 通道

// ============================================================
// 9. GPIO 配置
// ============================================================
#define SYS_OUT 16  // 电源按钮检测
#define BOOT    0    // BOOT 按钮
#define LCD_BL  8    // LCD 背光

// ============================================================
// 10. LCD 背光 PWM 模式
// ============================================================
#define LCD_PWM_MODE_0    0
#define LCD_PWM_MODE_100  100
#define LCD_PWM_MODE_150  150
#define LCD_PWM_MODE_255  255

// ============================================================
// 11. TCA9554 IO 扩展器引脚定义
// ============================================================
#define EXIO6_BIT 6  // 电源控制
#define EXIO7_BIT 7  // 功放控制

#endif // USER_CONFIG_H
