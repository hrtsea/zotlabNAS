/* RTOS Task Definitions - Hardware Development Template
 *
 * This file defines FreeRTOS tasks for hardware initialization and basic operations.
 * Add your application-specific tasks here.
 *
 * Core Assignment:
 *   - Core 0: LVGL display task (in lvgl_port)
 *   - Core 1: Audio, RTC, button input, battery monitoring
 */

#ifndef XTASK_H
#define XTASK_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// Forward declarations
void button_input_task(void *param);
void batt_level_read_task(void *param);
void rtc_read_task(void *param);
void audio_loop_task(void *param);
void cpu_monitor_task(void *param);

// Audio command queue (if audio is used)
extern QueueHandle_t audio_cmd_queue;

#endif  // XTASK_H
