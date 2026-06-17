// Fan PWM BSP - Hardware PWM fan control for ESP32-S3
// Supports temperature-based automatic control and manual PWM mode
#ifndef FAN_PWM_BSP_H
#define FAN_PWM_BSP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize fan PWM hardware (LEDC timer + channel)
// pwm_pct: initial duty cycle 0-100 (percentage)
void fan_pwm_bsp_init(uint8_t pwm_pct);

// Set fan speed as percentage (0-100)
// Uses ramp control for smooth transitions
void fan_pwm_bsp_set_pwm(uint8_t pwm_pct);

// Get current fan PWM percentage (0-100)
uint8_t fan_pwm_bsp_get_pwm(void);

// Emergency stop - immediately cut power to fan
void fan_pwm_bsp_emergency_stop(void);

// Get stall alarm state (true = fan may be stalled)
bool fan_pwm_bsp_get_stall_alarm(void);

// Clear stall alarm flag
void fan_pwm_bsp_clear_stall_alarm(void);

// Enable/disable fan control
void fan_pwm_bsp_set_enabled(bool enabled);

#ifdef __cplusplus
}
#endif

#endif // FAN_PWM_BSP_H
