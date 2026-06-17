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
#include <WiFi.h>
#include "data/config.h"
#include "net/wifi_manager.h"

// 外部变量声明
extern AppConfig g_config;
extern WiFiManager g_wifi;

extern TCA9554 *io;
extern QueueHandle_t audio_cmd_queue;

//==============================================
// BUTTON INPUT TASK - Core 1
void button_input_task(void *param) {
<<<<<<< HEAD
    static uint32_t last_boot_press_ms = 0;
    static uint32_t last_power_press_ms = 0;
    const uint32_t DEBOUNCE_MS = 50;
    
    for (;;) {
        uint32_t now = millis();
        
        // Check BOOT button
        if (digitalRead(BOOT) == LOW && now - last_boot_press_ms > DEBOUNCE_MS) {
            Serial.println("BOOT button pressed");
            last_boot_press_ms = now;
        }

        // Check power button
        if (digitalRead(SYS_OUT) == LOW && now - last_power_press_ms > DEBOUNCE_MS) {
            Serial.println("Power button pressed");
            last_power_press_ms = now;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
=======
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
>>>>>>> 89db8d722f90853fa0efe8e106b49eadd6220200
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

// 跟踪时间来源：true = PCF85063, false = ESP32 内部 RTC
static bool s_time_from_pcf85063 = true;
static uint32_t s_last_ntp_sync_ms = 0;
static const uint32_t NTP_SYNC_INTERVAL_MS = 3600000;  // 每小时同步一次

void rtc_read_task(void *param) {
  (void)param;
  
  // 初始化 PCF85063
  rtc.begin();
  Serial.printf("[RTC] PCF85063 %s\n", 
                s_time_from_pcf85063 ? "OK" : "FAILED, using ESP32 internal RTC");
  
  for (;;) {
    bool pcf_ok = false;
    
    // 1. 尝试从 PCF85063 读取
    if (s_time_from_pcf85063) {
      pcf_ok = rtc.getDateTime();
      if (!pcf_ok) {
        Serial.println(F("[RTC] PCF85063 read FAILED, switching to ESP32 internal RTC"));
        s_time_from_pcf85063 = false;
      }
    }
    
    // 2. 如果 PCF85063 不可用，使用 ESP32 内部 RTC
    if (!s_time_from_pcf85063) {
      time_t now_sec = time(nullptr);
      struct tm timeinfo;
      localtime_r(&now_sec, &timeinfo);
      
      // 更新全局 now 变量
      now.year = timeinfo.tm_year + 1900;
      now.month = timeinfo.tm_mon + 1;
      now.dayOfMonth = timeinfo.tm_mday;
      now.hour = timeinfo.tm_hour;
      now.minute = timeinfo.tm_min;
      now.second = timeinfo.tm_sec;
      now.dayOfWeek = timeinfo.tm_wday;  // 0=Sun
      
      // 每隔一段时间尝试重新连接 PCF85063
      static uint32_t last_pcf_retry_ms = 0;
      if (millis() - last_pcf_retry_ms > 60000) {  // 每分钟尝试一次
        last_pcf_retry_ms = millis();
        Serial.println(F("[RTC] Trying to reconnect PCF85063..."));
        rtc.begin();
        if (rtc.getDateTime()) {
          Serial.println(F("[RTC] PCF85063 reconnected! Switching back"));
          s_time_from_pcf85063 = true;
        }
      }
    }
    
    // 3. 如果 WiFi 已连接，定期用 NTP 同步 PCF85063
    if (WiFi.status() == WL_CONNECTED) {
      if (millis() - s_last_ntp_sync_ms > NTP_SYNC_INTERVAL_MS || s_last_ntp_sync_ms == 0) {
        s_last_ntp_sync_ms = millis();
        Serial.println(F("[RTC] Syncing PCF85063 with NTP..."));
        rtc.ntp_sync(g_config.timezone, 0);  // 使用配置中的时区
      }
    }
    
    // Debug output
    Serial.printf("[RTC] %s: %04d-%02d-%02d %02d:%02d:%02d (source: %s)\n",
                   s_time_from_pcf85063 ? "PCF85063" : "ESP32 RTC",
                   now.year, now.month, now.dayOfMonth,
                   now.hour, now.minute, now.second,
                   s_time_from_pcf85063 ? "PCF85063" : "ESP32");
    
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

//==============================================
// AUDIO INFO CALLBACK
#include "ESP32-audioI2S-master/Audio.h"
void my_audio_info(Audio::msg_t msg) {
  if (msg.msg) {
    Serial.printf("Audio [%d]: %s\n", msg.e, msg.msg);
  }
}

//==============================================
// AUDIO LOOP TASK - Core 1
extern Audio audio;
void audio_loop_task(void *param) {
  for (;;) {
    audio.loop();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

//==============================================
// CPU MONITOR TASK - Dual core usage
#include <esp_freertos_hooks.h>

static volatile uint64_t s_idle_count[2] = {0, 0};

bool IRAM_ATTR idle_hook0(void) {
<<<<<<< HEAD
  s_idle_count[0] = s_idle_count[0] + 1;
  return false;
}
bool IRAM_ATTR idle_hook1(void) {
  s_idle_count[1] = s_idle_count[1] + 1;
=======
  s_idle_count[0]++;
  return false;
}
bool IRAM_ATTR idle_hook1(void) {
  s_idle_count[1]++;
>>>>>>> 89db8d722f90853fa0efe8e106b49eadd6220200
  return false;
}

void cpu_monitor_task(void *param) {
  esp_register_freertos_idle_hook_for_cpu(idle_hook0, 0);
  esp_register_freertos_idle_hook_for_cpu(idle_hook1, 1);

  uint64_t last_count[2] = {0, 0};
  uint32_t last_ms = millis();
  float max_idle_per_ms = 0;

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(2000));

    uint32_t now_ms = millis();
    uint32_t elapsed = now_ms - last_ms;
    if (elapsed < 100) elapsed = 2000;

    uint64_t curr[2] = {s_idle_count[0], s_idle_count[1]};
    float idle_per_ms[2] = {
      (float)(curr[0] - last_count[0]) / elapsed,
      (float)(curr[1] - last_count[1]) / elapsed
    };

    for (int i = 0; i < 2; i++) {
      if (idle_per_ms[i] > max_idle_per_ms) {
        max_idle_per_ms = idle_per_ms[i];
      }
    }
    if (max_idle_per_ms < 1.0f) max_idle_per_ms = 1.0f;

    float cpu0 = 100.0f * (1.0f - idle_per_ms[0] / max_idle_per_ms);
    float cpu1 = 100.0f * (1.0f - idle_per_ms[1] / max_idle_per_ms);
    if (cpu0 < 0) cpu0 = 0; if (cpu0 > 100) cpu0 = 100;
    if (cpu1 < 0) cpu1 = 0; if (cpu1 > 100) cpu1 = 100;

    log_i("Core0: %.1f%%, Core1: %.1f%%", cpu0, cpu1);

    last_ms = now_ms;
    last_count[0] = curr[0];
    last_count[1] = curr[1];
  }
}
