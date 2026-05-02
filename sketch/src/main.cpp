/**
 * ESP32 Hardware Development Template
 * 
 * 这个模板提供了基本的硬件初始化框架，适用于 ESP32-S3 开发板。
 * 包含 I2C、SPI、GPIO、ADC 等常用外设的初始化示例。
 * 
 * 硬件平台: ESP32-S3 (可根据需要修改 platformio.ini)
 * 开发框架: Arduino (PlatformIO)
 */

#include <Arduino.h>
#include <stdio.h>

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

// FreeRTOS 队列（如需任务间通信）
QueueHandle_t sensor_queue = NULL;

// ############################################################
// SETUP: 硬件初始化
// ############################################################
void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(100);
  Serial.println("========================================");
  Serial.println("ESP32 Hardware Development Template");
  Serial.println("========================================");

  // 初始化随机种子
  randomSeed(esp_random());

  // 配置输入引脚
  pinMode(BOOT, INPUT_PULLUP);  // BOOT 按钮

  // 配置 ADC
  analogReadResolution(12);       // 9-13 bits 可选
  analogSetAttenuation(ADC_11db); // 满量程 ~3.3V

  // 初始化 I2C
  Serial.print("Initializing I2C...");
  i2c_master_Init();
  Serial.println("Done");

  // 初始化 IO 扩展器 (TCA9554)
  io = new TCA9554(tca9554_dev_handle);
  io->begin();
  Serial.println("TCA9554 IO Expander initialized");

  // 初始化 RTC (PCF85063)
  if (rtc.begin()) {
    Serial.println("PCF85063 RTC initialized");
    // 如果 RTC 时间丢失，设置为编译时间
    if (!rtc.isRunning()) {
      rtc.setDateTime(__DATE__, __TIME__);
      Serial.println("RTC time set to compile time");
    }
  } else {
    Serial.println("PCF85063 RTC initialization failed");
  }

  // 初始化 LCD 背光 (如硬件支持)
  // lcd_bl_pwm_bsp_init(LCD_PWM_MODE_255);
  // Serial.println("LCD backlight initialized");

  // 初始化触摸 (如硬件支持)
  // touch_init();
  // Serial.println("Touch controller initialized");

  // 创建 FreeRTOS 任务 (示例)
  //sensor_queue = xQueueCreate(10, sizeof(SensorData));
  //assert(sensor_queue != NULL);

  // 创建任务示例 (取消注释以启用)
  // xTaskCreatePinnedToCore(rtc_read_task, "RTC_Task", 3 * 1024, NULL, 3, NULL, 1);
  // xTaskCreatePinnedToCore(button_input_task, "Button_Task", 2 * 1024, NULL, 2, NULL, 1);
  // xTaskCreatePinnedToCore(sensor_read_task, "Sensor_Task", 4 * 1024, NULL, 1, NULL, 1);

  Serial.println("Setup complete. Entering main loop...");
}

// ############################################################
// LOOP: 主循环
// ############################################################
void loop() {
  // 示例: 读取 BOOT 按钮
  if (digitalRead(BOOT) == LOW) {
    Serial.println("BOOT button pressed");
    delay(500);  // 简单防抖
  }

  // 示例: 读取 ADC
  // int adcValue = analogRead(ADC_BATT);
  // float voltage = adcValue * 3.3f / 4095.0f;
  // Serial.printf("ADC: %d, Voltage: %.2fV\n", adcValue, voltage);

  // 示例: 读取 RTC 时间
  // if (rtc.getDateTime()) {
  //   Serial.printf("Time: %02d:%02d:%02d\n", rtc.hour, rtc.minute, rtc.second);
  // }

  // 延时 (避免串口输出过快)
  delay(1000);
}

// ############################################################
// FreeRTOS 任务示例
// ############################################################

// 任务: 读取 RTC 时间 (运行在 Core 1)
void rtc_read_task(void *param) {
  for (;;) {
    if (rtc.getDateTime()) {
      Serial.printf("[RTC Task] Time: %02d:%02d:%02d\n", 
                    rtc.hour, rtc.minute, rtc.second);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));  // 每秒更新
  }
}

// 任务: 按钮输入处理 (运行在 Core 1)
void button_input_task(void *param) {
  bool lastState = HIGH;
  for (;;) {
    bool currentState = digitalRead(BOOT);
    if (lastState == HIGH && currentState == LOW) {
      Serial.println("[Button Task] BOOT button pressed");
      // 在这里处理按钮事件
    }
    lastState = currentState;
    vTaskDelay(pdMS_TO_TICKS(10));  // 10ms 轮询
  }
}

// 任务: 传感器读取 (示例) (运行在 Core 1)
void sensor_read_task(void *param) {
  for (;;) {
    // 示例: 读取传感器数据
    // int sensorValue = analogRead(SENSOR_PIN);
    // Serial.printf("[Sensor Task] Value: %d\n", sensorValue);
    
    vTaskDelay(pdMS_TO_TICKS(100));  // 100ms 轮询
  }
}
