// Fan PWM BSP - Hardware PWM fan control implementation
// Uses ESP32 LEDC peripheral for 4-wire PWM fan control
// Frequency: 25kHz (standard for PWM fans)
// Duty cycle: 0-100% mapped to LEDC 8-bit resolution

#include <stdio.h>
#include <string.h>
#include "fan_pwm_bsp.h"
#include "esp_err.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

// GPIO_NUM_NC may not be defined in all versions
#ifndef GPIO_NUM_NC
#define GPIO_NUM_NC (-1)
#endif

#include "user_config.h"

// ===== Configuration (LEDC timer & channel) =====
// Use LEDC_TIMER_0 & LEDC_CHANNEL_0 to avoid conflict with backlight (timer3/channel1)
#define FAN_LEDC_SPEED_MODE   LEDC_LOW_SPEED_MODE
#define FAN_LEDC_TIMER        LEDC_TIMER_0
#define FAN_LEDC_CHANNEL      LEDC_CHANNEL_0
#define FAN_LEDC_RESOLUTION   LEDC_TIMER_8_BIT   // 0-255
#define FAN_LEDC_FREQ_HZ      25000               // 25kHz standard for PWM fans

// PWM percentage to duty map (0-100% -> 0-255)
#define FAN_PWM_MAX_DUTY      ((1 << 8) - 1)      // 255

// Runtime state
static volatile uint8_t  s_current_pwm_pct = 0;
static volatile uint8_t  s_target_pwm_pct  = 0;
static volatile bool     s_enabled          = false;
static volatile bool     s_stall_alarm      = false;
static volatile uint32_t s_last_update_ms   = 0;
static volatile bool     s_initialized      = false;

// Convert percentage (0-100) to LEDC duty (0-255)
static inline uint16_t pct_to_duty(uint8_t pct) {
    if (pct > 100) pct = 100;
    return (uint16_t)((uint32_t)pct * FAN_PWM_MAX_DUTY / 100);
}

// Apply duty cycle to hardware
static void apply_duty(uint16_t duty) {
    esp_err_t err;
    err = ledc_set_duty(FAN_LEDC_SPEED_MODE, FAN_LEDC_CHANNEL, duty);
    if (err != ESP_OK) {
        printf("[FanPWM] ledc_set_duty failed: %s\n", esp_err_to_name(err));
        return;
    }
    err = ledc_update_duty(FAN_LEDC_SPEED_MODE, FAN_LEDC_CHANNEL);
    if (err != ESP_OK) {
        printf("[FanPWM] ledc_update_duty failed: %s\n", esp_err_to_name(err));
    }
}

// C-linkage - GPIO init helper
static void fan_gpio_init(void) {
    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask = ((uint64_t)0X01 << FAN_PWM_PIN);
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
}

// Initialize LEDC timer and channel for fan PWM
void fan_pwm_bsp_init(uint8_t pwm_pct) {
    if (s_initialized) {
        printf("[FanPWM] Already initialized\n");
        fan_pwm_bsp_set_pwm(pwm_pct);
        return;
    }

    // Configure GPIO
    fan_gpio_init();

    // Configure LEDC timer
    ledc_timer_config_t timer_conf = {
        .speed_mode = FAN_LEDC_SPEED_MODE,
        .duty_resolution = FAN_LEDC_RESOLUTION,
        .timer_num = FAN_LEDC_TIMER,
        .freq_hz = FAN_LEDC_FREQ_HZ,
        .clk_cfg = LEDC_SLOW_CLK_RC_FAST,
    };
    ESP_ERROR_CHECK_WITHOUT_ABORT(ledc_timer_config(&timer_conf));

    // Configure LEDC channel
    ledc_channel_config_t ledc_conf = {
        .gpio_num = FAN_PWM_PIN,
        .speed_mode = FAN_LEDC_SPEED_MODE,
        .channel = FAN_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = FAN_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK_WITHOUT_ABORT(ledc_channel_config(&ledc_conf));

    s_initialized = true;
    s_enabled = true;
    s_current_pwm_pct = 0;
    s_target_pwm_pct = (pwm_pct > 100) ? 100 : pwm_pct;

    printf("[FanPWM] Initialized (pin=%d, freq=%dkHz, initial=%d%%)\n",
           FAN_PWM_PIN, FAN_LEDC_FREQ_HZ / 1000, s_target_pwm_pct);

    // Apply initial duty
    apply_duty(pct_to_duty(s_target_pwm_pct));
    s_current_pwm_pct = s_target_pwm_pct;
}

// Set fan speed with clamping
void fan_pwm_bsp_set_pwm(uint8_t pwm_pct) {
    if (!s_initialized) {
        printf("[FanPWM] WARN: not initialized, call fan_pwm_bsp_init() first\n");
        return;
    }
    if (!s_enabled) {
        return;
    }
    if (pwm_pct > 100) pwm_pct = 100;

    s_target_pwm_pct = pwm_pct;

    // Simple direct set (no ramp on ESP32 - we'll do soft-ramp in task loop)
    apply_duty(pct_to_duty(pwm_pct));
    s_current_pwm_pct = pwm_pct;
}

uint8_t fan_pwm_bsp_get_pwm(void) {
    return s_current_pwm_pct;
}

void fan_pwm_bsp_emergency_stop(void) {
    if (!s_initialized) return;
    printf("[FanPWM] EMERGENCY STOP!\n");
    apply_duty(0);
    s_current_pwm_pct = 0;
    s_target_pwm_pct = 0;
}

bool fan_pwm_bsp_get_stall_alarm(void) {
    return s_stall_alarm;
}

void fan_pwm_bsp_clear_stall_alarm(void) {
    s_stall_alarm = false;
}

void fan_pwm_bsp_set_enabled(bool enabled) {
    if (!s_initialized) return;
    s_enabled = enabled;
    if (!enabled) {
        apply_duty(0);
        s_current_pwm_pct = 0;
    }
}
