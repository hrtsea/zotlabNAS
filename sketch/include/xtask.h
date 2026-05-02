/* ESP32 Hardware Development Template - FreeRTOS 任务定义
 * 
 * 这个文件定义了 FreeRTOS 任务函数和示例。
 * 根据你的项目需求修改或扩展这些任务。
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ============================================================
// 任务声明 (在 main.cpp 中实现)
// ============================================================

// 任务: 读取 RTC 时间 (运行在 Core 1)
void rtc_read_task(void *param);

// 任务: 按钮输入处理 (运行在 Core 1)
void button_input_task(void *param);

// 任务: 传感器读取示例 (运行在 Core 1)
void sensor_read_task(void *param);

// ============================================================
// 任务实现示例 (简单版本，可在 main.cpp 中重写)
// ============================================================

// 任务: 读取 RTC 时间 (示例实现)
/*
void rtc_read_task(void *param) {
  for (;;) {
    // 在这里实现 RTC 读取逻辑
    // 示例: Serial.printf("RTC Task running\n");
    vTaskDelay(pdMS_TO_TICKS(1000));  // 每秒执行一次
  }
}
*/

// 任务: 按钮输入处理 (示例实现)
/*
void button_input_task(void *param) {
  bool lastState = HIGH;
  for (;;) {
    bool currentState = digitalRead(BOOT);
    if (lastState == HIGH && currentState == LOW) {
      Serial.println("Button pressed");
      // 在这里处理按钮事件
    }
    lastState = currentState;
    vTaskDelay(pdMS_TO_TICKS(10));  // 10ms 轮询
  }
}
*/

// 任务: 传感器读取 (示例实现)
/*
void sensor_read_task(void *param) {
  for (;;) {
    // 在这里实现传感器读取逻辑
    // 示例: int value = analogRead(SENSOR_PIN);
    // Serial.printf("Sensor value: %d\n", value);
    vTaskDelay(pdMS_TO_TICKS(100));  // 100ms 轮询
  }
}
*/

// ============================================================
// 任务创建示例 (在 setup() 中调用)
// ============================================================
/*
  // 创建 RTC 读取任务 (Core 1, 优先级 3)
  xTaskCreatePinnedToCore(
    rtc_read_task,      // 任务函数
    "RTC_Task",        // 任务名称
    3 * 1024,          // 堆栈大小 (bytes)
    NULL,              // 任务参数
    3,                 // 优先级
    NULL,              // 任务句柄
    1                  // 运行在 Core 1
  );

  // 创建按钮输入任务 (Core 1, 优先级 2)
  xTaskCreatePinnedToCore(
    button_input_task,
    "Button_Task",
    2 * 1024,
    NULL,
    2,
    NULL,
    1
  );

  // 创建传感器读取任务 (Core 1, 优先级 1)
  xTaskCreatePinnedToCore(
    sensor_read_task,
    "Sensor_Task",
    4 * 1024,
    NULL,
    1,
    NULL,
    1
  );
*/
