/**
 * @details
 *  - **Application Name:** TuneBar Hardware Template
 *  - **Developed By:** Va&Cob (modified)
 *
 *  **Hardware Platform**
 *  - **Board Model:** Waveshare ESP32-S3-Touch-LCD-3.49
 *     (3.49″ IPS capacitive touch, 172 × 640 resolution, QSPI display interface)
 *     Reference: https://www.waveshare.com/product/arduino/boards-kits/esp32-s3/esp32-s3-touch-lcd-3.49.htm?sku=32374
 *
 *  - **Flash Size:** 16 MB
 *  - **PSRAM:** OPI PSRAM (enabled)
 *  - **USB CDC on Boot:** Enabled
 *
 * This is a hardware development template with all UI and audio logic removed.
 * Retained: I2C, SPI, GPIO, ADC, LCD backlight, touch, RTC, IO expander.
 */
#include "esp_heap_caps.h"
#include "mbedtls/platform.h"

// 硬件驱动
#include "i2c_bsp/i2c_bsp.h"
#include "lcd_bl_bsp/lcd_bl_pwm_bsp.h"
#include "pcf85063/pcf85063.h"
#include "tca9554/tca9554.h"
#include "touch/touch.h"

// 配置文件
#include "user_config.h"
#include "xtask.h"

// 全局对象
extern i2c_master_dev_handle_t tca9554_dev_handle;
TCA9554 *io = nullptr;
PCF85063 rtc;

// FreeRTOS 队列
QueueHandle_t sensor_queue = NULL;

// ############################################################
void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(100);
  Serial.println("========================================");
  Serial.println("ESP32-S3 Hardware Template");
  Serial.println("Waveshare ESP32-S3-Touch-LCD-3.49");
  Serial.println("========================================");

  // 初始化随机种子
  randomSeed(esp_random());

  // 配置输入引脚
  pinMode(BOOT, INPUT_PULLUP);  // BOOT 按钮
  pinMode(SYS_OUT, INPUT_PULLUP); // 电源按钮检测

  // 配置 ADC
  analogReadResolution(12);       // 9–13 bits 支持
  analogSetAttenuation(ADC_11db); // 满量程 ~3.3V

  // 初始化 I2C
  i2c_master_Init();
  Serial.println("I2C initialized");

  // 初始化 IO 扩展器 (TCA9554)
  io = new TCA9554(tca9554_dev_handle);
  io->begin();
  io->setPinMode(EXIO6_BIT, 0); // 设置输出模式
  Serial.println("TCA9554 IO Expander initialized");

  // 电源按钮处理
  if (digitalRead(SYS_OUT) == LOW) {
    Serial.println("< POWER ON >");
    io->digitalWrite(EXIO6_BIT, 1); // 保持开机
  }

  // 初始化 RTC (PCF85063)
  if (rtc.begin()) {
    Serial.println("PCF85063 RTC initialized");
    if (!rtc.isRunning()) {
      rtc.setDateTime(__DATE__, __TIME__);
      Serial.println("RTC time set to compile time");
    }
  } else {
    Serial.println("PCF85063 RTC initialization failed");
  }

  // 初始化 LCD 背光
  lcd_bl_pwm_bsp_init(LCD_PWM_MODE_255); // 最大亮度
  Serial.println("LCD backlight initialized");

  // 初始化触摸 (如硬件支持)
  // touch_init();
  // Serial.println("Touch controller initialized");

  // 创建 FreeRTOS 任务示例
  // xTaskCreatePinnedToCore(rtc_read_task, "RTC_Task", 3 * 1024, NULL, 3, NULL, 1);
  // xTaskCreatePinnedToCore(button_input_task, "Button_Task", 2 * 1024, NULL, 2, NULL, 1);
  // xTaskCreatePinnedToCore(batt_level_read_task, "Battery_Task", 2 * 1024, NULL, 1, NULL, 1);

  Serial.println("Setup complete. Entering main loop...");
}

// ############################################################
void loop() {
  // 示例: 读取 BOOT 按钮
  if (digitalRead(BOOT) == LOW) {
    Serial.println("BOOT button pressed");
    delay(500);  // 简单防抖
  }

  // 示例: 读取 ADC (电池电压)
  // int adcValue = analogRead(ADC_BATT);
  // float voltage = adcValue * 3.3f / 4095.0f;
  // Serial.printf("ADC: %d, Voltage: %.2fV\n", adcValue, voltage);

  // 示例: 读取 RTC 时间
  // if (rtc.getDateTime()) {
  //   Serial.printf("Time: %02d:%02d:%02d\n", rtc.hour, rtc.minute, rtc.second);
  // }

  delay(1000);  // 延时 (避免串口输出过快)
}
