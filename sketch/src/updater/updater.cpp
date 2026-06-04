#include "lcd_bl_bsp/lcd_bl_pwm_bsp.h"
#include "net/network.h"
#include "ui/ui.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_ota_ops.h>

const char *compile_date = __DATE__ " - " __TIME__;
const char *current_version = "1.2.0";

bool firmware_checked = false; 

static TaskHandle_t otaTaskHandle = NULL;
static lv_obj_t *modal_blocker = NULL;
static lv_obj_t *ota_popup = NULL;
static lv_obj_t *ota_bar = NULL;
static lv_obj_t *ota_msg = NULL;
static lv_obj_t *btn_close = NULL;
static lv_obj_t *lbl_update = NULL;

static volatile bool ota_abort = false;

const char *newFirmwareAvailable() { 
  log_i("Firmware check disabled for this project");
  return nullptr;
}

void ota_show_popup() {
}

void notifyUpdate(const char *latestVer) {
}

void memoryInfo(char *buf, size_t len) {
  snprintf(buf, len, "Memory: Not available");
}

void systemInfo(char *buf, size_t len) {
  uint64_t mac = ESP.getEfuseMac();
  snprintf(buf, len,
           "[ TuneBar by Va&Cob ]\n"
           "BUILD  : %s - %s\n"
           "SERIAL : %012" PRIx64,
           current_version, compile_date, mac);
}