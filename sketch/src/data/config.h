// ZotLab NAS Monitor - NVS Configuration Management
#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "nas_data.h"
#include "nas_config.h"

#define NVS_NAMESPACE "nasmon"
#define NVS_WIFI_SSID "ssid"
#define NVS_WIFI_PASS "wifipass"
#define NVS_NAS_TYPE "nas_type"
#define NVS_NAS_IP "nas_ip"
#define NVS_NAS_PORT "nas_port"
#define NVS_NAS_USER "nas_user"
#define NVS_NAS_PASS "nas_pass"
#define NVS_NAS_HTTPS "nas_https"
#define NVS_SNMP_COMM "snmp_comm"
#define NVS_SNMP_VER "snmp_ver"
#define NVS_SERIAL_BAUD "serial_baud"
#define NVS_POLL_SEC "poll_sec"
#define NVS_ROTATION_ANGLE "rotation_angle"  // 0=0°, 1=90°, 2=180°, 3=270°
#define NVS_BRIGHTNESS "brightness"
#define NVS_AUTODIM "autodim"
#define NVS_TIMEZONE "timezone"
#define NVS_SATA_DISK_COUNT "sata_disk_count"  // SATA 硬盘数量
#define NVS_M2_DISK_COUNT "m2_disk_count"      // M.2 硬盘数量

// Weather NVS keys
#define NVS_WEATHER_API_KEY "weather_api"
#define NVS_WEATHER_CITY "weather_city"

// Auto Cycle NVS keys (max 15 chars)
#define NVS_AUTO_CYCLE_ENABLE "autocycle_en"
#define NVS_AUTO_CYCLE_INTERVAL "autocycle_int"

// Fan NVS keys
#define NVS_FAN_ENABLE "fan_enable"
#define NVS_FAN_MODE "fan_mode"
#define NVS_FAN_MANUAL "fan_manual"
#define NVS_FAN_CURVE "fan_curve"
#define NVS_FAN_TSRC "fan_tsrc"
#define NVS_FAN_HYST "fan_hyst"
#define NVS_FAN_MIN "fan_min"
#define NVS_FAN_EMERG "fan_emerg"
#define NVS_FAN_RAMP "fan_ramp"
#define NVS_FAN_STALL "fan_stall"

struct AppConfig {
    // WiFi
    char ssid[33];
    char wifipass[65];

    // NAS
    char nas_type[16];
    char nas_ip[40];
    uint16_t nas_port;
    char nas_user[32];
    char nas_pass[65];
    bool nas_https;
    char snmp_comm[32];
    uint8_t snmp_ver;  // SNMP version (1=v1, 2=v2c, 3=v3)
    uint32_t serial_baud;

    // Polling & Display
    uint8_t poll_sec;
    uint8_t rotation_angle;  // 0=0°, 1=90°(portrait), 2=180°, 3=270°(portrait flipped)
    uint8_t brightness;
    bool autodim;
    int8_t timezone;
    
    // Auto Cycle
    bool auto_cycle_enabled;           // 自动循环启用状态
    uint8_t auto_cycle_interval_sec;   // 循环时间间隔(秒)
    
    // Hardware
    uint8_t sata_disk_count;  // SATA 硬盘数量（从 NVS 加载，默认 DEFAULT_SATA_DISK_COUNT）
    uint8_t m2_disk_count;    // M.2 硬盘数量（从 NVS 加载，默认 DEFAULT_M2_DISK_COUNT）

    // Weather
    char weather_api_key[65];
    char weather_city[32];

    // Fan
    FanConfig fan;  // 风扇配置（使用 FanConfig 结构体）
};

// 配置备份结构（用于回退机制）
struct ConfigBackup {
    char nas_type[16];
    char nas_ip[40];
    uint16_t nas_port;
    char nas_user[32];
    char nas_pass[65];
    bool nas_https;
    char snmp_comm[32];
    uint8_t snmp_ver;
    uint32_t serial_baud;
};

// 全局配置备份
extern ConfigBackup g_config_backup;

// 保存当前配置到备份
void config_save_backup();

// 从备份恢复配置
void config_restore_backup();

// 配置管理函数
extern AppConfig g_config;

void config_load();
void config_save();
void config_save_wifi(const char* ssid, const char* pass);
void config_save_nas(const char* type, const char* ip, uint16_t port,
                     const char* user, const char* pass, bool https);
void config_save_display(uint8_t rotation_angle, uint8_t brightness, bool autodim);
void config_save_fan(const FanConfig& fan);
void config_save_disk_config(uint8_t sata_count, uint8_t m2_count);  // 保存磁盘配置
void config_reset();
