/* RTOS Task Implementation - Hardware Development Template
 *
 * This file implements basic hardware tasks.
 * Modify these tasks to suit your application.
 */

#include "xtask.h"
#include "user_config.h"
#include "pcf85063/pcf85063.h"
#include "tca9554/tca9554.h"
#include <Arduino.h>

extern TCA9554 *io;
QueueHandle_t audio_cmd_queue = NULL;

//==============================================
// BUTTON INPUT TASK - Core 1
void button_input_task(void *param) {
  for (;;) {
    // Check BOOT button
    if (digitalRead(BOOT) == LOW) {
      Serial.println("BOOT button pressed");
      vTaskDelay(pdMS_TO_TICKS(200));  // Debounce
    }

    // Check power button
    if (digitalRead(SYS_OUT) == LOW) {
      Serial.println("Power button pressed");
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

//==============================================
// BATTERY LEVEL READ TASK - Core 1
void batt_level_read_task(void *param) {
  for (;;) {
    int rawValue = analogReadMilliVolts(ADC_BATT);
    float vbat = rawValue * 3.0f / 1000.0f;
    Serial.printf("Battery: %.2fV (raw: %d)\n", vbat, rawValue);
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}

//==============================================
// RTC READ TASK - Core 1
extern PCF85063 rtc;
extern RTC_DateTime now;  // Global variable from pcf85063.cpp
void rtc_read_task(void *param) {
  rtc.begin();
  for (;;) {
    if (rtc.getDateTime()) {
      Serial.printf("RTC: %04d-%02d-%02d %02d:%02d:%02d\n",
                     now.year, now.month, now.dayOfMonth,
                     now.hour, now.minute, now.second);
    } else {
      Serial.println("RTC read error");
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

//==============================================
// AUDIO LOOP TASK - Core 1
#include "ESP32-audioI2S-master/Audio.h"
extern Audio audio;
void audio_loop_task(void *param) {
  for (;;) {
    audio.loop();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
