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
#include "fan_pwm_bsp.h"
#include "net/data_source.h"

// 外部变量声明
extern AppConfig g_config;
extern WiFiManager g_wifi;

extern TCA9554 *io;
extern QueueHandle_t audio_cmd_queue;

//==============================================
// BUTTON INPUT TASK - Core 1
void button_input_task(void *param) {
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
  s_idle_count[0]++;
  return false;
}
bool IRAM_ATTR idle_hook1(void) {
  s_idle_count[1]++;
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

//==============================================
// FAN CONTROL TASK - Core 1
// Temperature-based PWM fan control with hysteresis and ramp control
// Reads NAS temperature data and applies configured fan curve
void fan_control_task(void *param) {
  (void)param;
  static uint8_t last_pwm_pct = 0;
  static int16_t last_temp = 0;
  static uint32_t last_stall_check_ms = 0;

  // Initialize fan PWM hardware
  uint8_t initial_pwm = g_config.fan.enabled ? g_config.fan.manual_pwm_pct : 0;
  fan_pwm_bsp_init(initial_pwm);
  fan_pwm_bsp_set_enabled(g_config.fan.enabled);

  Serial.printf("[Fan] Control task started (mode=%s, initial=%d%%, curve_points=%d)\n",
                g_config.fan.mode == 0 ? "AUTO" : "MANUAL",
                initial_pwm, FAN_CURVE_POINTS);

  for (;;) {
    // Fan enabled flag from config (can change at runtime via UI)
    if (!g_config.fan.enabled) {
      fan_pwm_bsp_set_pwm(0);
      last_pwm_pct = 0;
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    // ===== Step 1: Determine target temperature =====
    int16_t ctrl_temp = 0;
    bool temp_valid = false;

    if (g_data_source != nullptr && g_data_source->isConnected()) {
      const NasData& data = g_data_source->getData();

      // Select temperature source based on config
      switch (g_config.fan.temp_source) {
        case 0:  // TEMP_MAX_CPU_SYS
          ctrl_temp = (int16_t)max((int16_t)data.system.temp_cpu,
                                   (int16_t)data.system.temp_sys);
          temp_valid = (ctrl_temp > 0);
          break;
        case 1:  // TEMP_AVG_CPU_SYS
          if (data.system.temp_cpu > 0 && data.system.temp_sys > 0) {
            ctrl_temp = (int16_t)((data.system.temp_cpu + data.system.temp_sys) / 2);
            temp_valid = true;
          } else if (data.system.temp_cpu > 0) {
            ctrl_temp = data.system.temp_cpu;
            temp_valid = true;
          } else if (data.system.temp_sys > 0) {
            ctrl_temp = data.system.temp_sys;
            temp_valid = true;
          }
          break;
        case 2:  // TEMP_CPU_ONLY
          ctrl_temp = data.system.temp_cpu;
          temp_valid = (ctrl_temp > 0);
          break;
        case 3:  // TEMP_SYS_ONLY
          ctrl_temp = data.system.temp_sys;
          temp_valid = (ctrl_temp > 0);
          break;
        default:
          ctrl_temp = (int16_t)max((int16_t)data.system.temp_cpu,
                                   (int16_t)data.system.temp_sys);
          temp_valid = (ctrl_temp > 0);
          break;
      }
    }

    // Fallback: no temperature data - use manual or default minimum
    if (!temp_valid) {
      // No NAS data available - use manual PWM or safe minimum
      if (g_config.fan.mode == 1) {  // FAN_MODE_MANUAL
        uint8_t target = g_config.fan.manual_pwm_pct;
        if (target != last_pwm_pct) {
          fan_pwm_bsp_set_pwm(target);
          last_pwm_pct = target;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        continue;
      }
      // AUTO without temp data - use min PWM (safe default, not 0 to prevent overheat)
      ctrl_temp = 35;  // Assume moderate temperature
      temp_valid = true;
    }

    // ===== Step 2: Determine target PWM =====
    uint8_t target_pwm = 0;

    if (g_config.fan.mode == 1) {
      // MANUAL mode - use configured PWM directly
      target_pwm = g_config.fan.manual_pwm_pct;
    } else {
      // AUTO mode - interpolate fan curve
      // Fan curve: array of (temp, pwm_pct) points, sorted by temp ascending
      // Points from config: config_load memcpy DEFAULT_FAN_CURVE as default
      FanCurvePoint* curve = (FanCurvePoint*)g_config.fan.curve;
      int curve_points = FAN_CURVE_POINTS;

      if (ctrl_temp <= curve[0].temp) {
        // Below lowest curve point - use minimum PWM (not 0, for safety)
        target_pwm = g_config.fan.min_pwm_pct;
      } else if (ctrl_temp >= curve[curve_points - 1].temp) {
        // Above highest curve point - full speed
        target_pwm = 100;
      } else {
        // Interpolate between curve points
        for (int i = 0; i < curve_points - 1; i++) {
          if (ctrl_temp >= curve[i].temp && ctrl_temp <= curve[i + 1].temp) {
            // Linear interpolation between curve[i] and curve[i+1]
            int16_t temp_range = curve[i + 1].temp - curve[i].temp;
            int16_t temp_delta = ctrl_temp - curve[i].temp;
            int16_t pwm_range = curve[i + 1].pwm_pct - curve[i].pwm_pct;
            if (temp_range > 0) {
              target_pwm = curve[i].pwm_pct +
                           (uint8_t)((int32_t)temp_delta * pwm_range / temp_range);
            } else {
              target_pwm = curve[i].pwm_pct;
            }
            break;
          }
        }
      }

      // Clamp to min PWM
      if (target_pwm < g_config.fan.min_pwm_pct) {
        target_pwm = g_config.fan.min_pwm_pct;
      }
      // Emergency over-temperature - full speed
      if (ctrl_temp >= g_config.fan.emergency_temp) {
        target_pwm = 100;
      }
    }

    // Clamp PWM to 0-100
    if (target_pwm > 100) target_pwm = 100;

    // ===== Step 3: Apply hysteresis (reduces PWM hunting) =====
    // Only change PWM if:
    // 1. Delta >= min_change_pct, OR
    // 2. Temperature delta >= hysteresis, OR
    // 3. Emergency condition
    int16_t temp_delta = ctrl_temp - last_temp;
    if (temp_delta < 0) temp_delta = -temp_delta;
    int16_t pwm_delta = (int16_t)target_pwm - (int16_t)last_pwm_pct;
    if (pwm_delta < 0) pwm_delta = -pwm_delta;

    bool should_update = false;
    if (last_pwm_pct == 0 || target_pwm == 100 || target_pwm == 0) {
      should_update = true;  // Always update on-state transitions
    } else if (pwm_delta >= g_config.fan.min_change_pct) {
      should_update = true;
    } else if (temp_delta >= g_config.fan.hysteresis) {
      should_update = true;
    }

    if (should_update) {
      // Soft ramp: incrementally approach target over multiple ticks
      // to prevent sudden current spikes. Ramp speed: 5% per 100ms
      uint8_t current = last_pwm_pct;
      if (current == target_pwm) {
        // already at target - just apply
        fan_pwm_bsp_set_pwm(target_pwm);
      } else {
        // Gradual ramp via set_pwm in loop
        while (current != target_pwm) {
          if (current < target_pwm) {
            current = (uint8_t)min((int)target_pwm, (int)current + 5);
          } else {
            current = (uint8_t)max((int)target_pwm, (int)current - 5);
          }
          fan_pwm_bsp_set_pwm(current);
          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }
      last_pwm_pct = target_pwm;
      last_temp = ctrl_temp;

      // Debug output (throttled)
      static uint32_t last_log_ms = 0;
      if (millis() - last_log_ms > 5000) {
        last_log_ms = millis();
        Serial.printf("[Fan] T=%d°C, PWM=%d%% (mode=%s, source=%d)\n",
                      ctrl_temp, target_pwm,
                      g_config.fan.mode == 0 ? "AUTO" : "MANUAL",
                      g_config.fan.temp_source);
      }
    }

    // ===== Step 4: Stall detection (simple heuristic) =====
    // If PWM > 0 but rpm is 0 (or very low), flag stall alarm
    // Actual tachometer reading would require pulse counter ISR
    if (g_config.fan.stall_detect_sec > 0 && target_pwm > 20) {
      uint32_t now = millis();
      if (now - last_stall_check_ms > (uint32_t)g_config.fan.stall_detect_sec * 1000) {
        last_stall_check_ms = now;
        // Simple software check: verify PWM was actually applied
        if (fan_pwm_bsp_get_pwm() == 0 && target_pwm > 20) {
          // This should not happen - log warning
          Serial.println(F("[Fan] WARN: target>0 but hardware reports PWM=0"));
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
