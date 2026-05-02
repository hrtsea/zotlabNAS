/**
 * @brief ESP32-S3 Hardware Development Template
 * @details
 *  - **Target Hardware:** Waveshare ESP32-S3-Touch-LCD-3.49
 *  - **Display:** AXS15231B (QSPI, 172x640)
 *  - **Touch:** I2C capacitive touch
 *  - **Audio:** ES8311 (DAC) + ES7210 (ADC)
 *  - **RTC:** PCF85063
 *  - **IO Expander:** TCA9554
 *
 * **Software Stack**
 *  - PlatformIO pioArduino
 *  - ESP32 Core 3.x
 *  - LVGL 8.3.x (optional UI)
 *  - ESP32 Audio I2S library
 */

#include "esp_heap_caps.h"
#include "mbedtls/platform.h"

// Hardware drivers
#include "i2c_bsp/i2c_bsp.h"
#include "lcd_bl_bsp/lcd_bl_pwm_bsp.h"
#include "lvgl_port/lvgl_port.h"
#include "pcf85063/pcf85063.h"
#include "tca9554/tca9554.h"
#include "touch/esp_lcd_touch.h"
#include "user_config.h"

#include <Arduino.h>
#include <stdio.h>
#include "lvgl.h"

// Audio drivers
#include "ESP32-audioI2S-master/Audio.h"
#include "es7210/es7210.h"
#include "es8311/es8311.h"
#include "ui/screens/ui_Screen_Boot.h"
#include "ui/screens/ui_Screen_Overview.h"

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// Audio global objects
Audio audio;
ES8311 speaker;
ES7210 mic;

// RTC object
PCF85063 rtc;

// IO Expander
extern i2c_master_dev_handle_t tca9554_dev_handle;
TCA9554 *io = nullptr;

// Audio command queue
QueueHandle_t audio_cmd_queue = NULL;

// External task declarations (defined in xtask.cpp)
extern void my_audio_info(Audio::msg_t msg);
extern void audio_loop_task(void *pvParameters);
extern void rtc_read_task(void *pvParameters);
extern void button_input_task(void *pvParameters);
extern void batt_level_read_task(void *pvParameters);

// ############################################################
void setup() {
  // Initialize serial
  Serial.begin(115200);
  delay(100);
  Serial.println("ESP32-S3 Hardware Template Starting...");

  // Input pins
  pinMode(BOOT, INPUT_PULLUP);
  pinMode(SYS_OUT, INPUT_PULLUP);

  // Setup ADC
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // Initialize I2C
  i2c_master_Init();

  // Initialize IO expander
  io = new TCA9554(tca9554_dev_handle);
  io->begin();
  io->setPinMode(EXIO6_BIT, 0);  // Output: power hold
  io->setPinMode(EXIO7_BIT, 0);  // Output: power amp

  // Power on hold
  if (digitalRead(SYS_OUT) == LOW) {
    Serial.println("Power ON");
    io->digitalWrite(EXIO6_BIT, 1);
  }

  // Initialize LVGL (display + touch)
  lvgl_port_init();

  // Show splash screen via LVGL timer (runs in LVGL main thread, no lock needed)
  lv_timer_create([](lv_timer_t *timer) {
    ui_Screen_Boot_screen_init();
    lv_scr_load(ui_Screen_Boot);
    lv_timer_del(timer);
  }, 1, NULL);

  // Initialize LCD backlight
  lcd_bl_pwm_bsp_init(LCD_PWM_MODE_255);

  // Enable power amplifier
  io->digitalWrite(EXIO7_BIT, 1);
  delay(100);
  if (io->digitalRead(EXIO7_BIT) == 0)
    Serial.println("Power Amp ERROR!");
  else
    Serial.println("Power Amp OK");

  // Initialize ES8311 (audio DAC)
  speaker.setVolume(80);
  if (!speaker.begin())
    Serial.println("ES8311 ERROR!");
  else
    Serial.println("ES8311 OK");

  // Initialize ES7210 (microphone ADC)
  if (mic.init()) {
    Serial.println("ES7210 OK");
  } else {
    Serial.println("ES7210 ERROR!");
  }

  // Initialize Audio library
  Audio::audio_info_callback = my_audio_info;
  audio.setAudioTaskCore(1);
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DSOUT, I2S_MCLK, I2S_DSIN);
  audio.forceMono(true);
  audio.setConnectionTimeout(2000, 4000);

  // Create audio command queue
  audio_cmd_queue = xQueueCreate(10, sizeof(int));
  assert(audio_cmd_queue != NULL);

  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(audio_loop_task, "audio_loop", 5 * 1024, NULL, 4, NULL, 1);
  xTaskCreatePinnedToCore(rtc_read_task, "rtc_read", 3 * 1024, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(button_input_task, "button_input", 4 * 1024, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(batt_level_read_task, "batt_read", 4 * 1024, NULL, 1, NULL, 1);

  Serial.println("Setup complete!");
}

// ############################################################
void loop() {
  // Main loop - LVGL tasks run in separate task (lvgl_port)
  vTaskDelay(pdMS_TO_TICKS(1000));
}
