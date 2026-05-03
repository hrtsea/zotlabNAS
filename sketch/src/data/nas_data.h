// ZotLab NAS Monitor - Unified Data Model
// All data sources fill this structure; UI only consumes this structure.
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "nas_config.h"

enum NasType {
    NAS_SYNOLOGY,
    NAS_QNAP,
    NAS_TRUENAS,
    NAS_FNOS,       // 飞牛NAS
    NAS_UNRAID,     // Unraid
    NET_LINUX_HTTP,
    NET_LINUX_SERIAL,
    NET_WINDOWS,
    NET_NETDATA,
    NET_SNMP,
    NAS_MOCK,  // Mock数据源（用于测试）
    NAS_TYPE_ENUM_COUNT  // renamed to avoid conflict with data_source.h
};

enum HealthStatus {
    HEALTH_OK,
    HEALTH_WARNING,
    HEALTH_CRITICAL,
    HEALTH_UNKNOWN
};

enum FanMode {
    FAN_MODE_AUTO = 0,
    FAN_MODE_MANUAL = 1
};

enum TempSource {
    TEMP_MAX_CPU_SYS = 0,
    TEMP_AVG_CPU_SYS = 1,
    TEMP_CPU_ONLY    = 2,
    TEMP_SYS_ONLY    = 3
};

struct NasSystemInfo {
    char hostname[32];
    char model[32];
    uint32_t uptime_s;
    float cpu_pct;
    float ram_pct;
    uint32_t ram_total_mb;
    uint32_t ram_used_mb;
    uint32_t ram_free_mb;
    uint32_t ram_cached_mb;
    uint32_t swap_total_mb;
    uint32_t swap_used_mb;
    int16_t temp_cpu;
    int16_t temp_sys;
    float cpu_cores[8];
    uint8_t cpu_core_count;
    float load_avg[3];
};

struct NasDiskInfo {
    char name[16];
    char device[32];        // 设备路径（如 /dev/sda）
    char model_name[32];
    char mount[32];
    char disk_type[8];      // SSD 或 HDD
    int16_t temp;
    HealthStatus health;
    uint32_t size_gb;
    uint32_t used_gb;
    uint8_t used_pct;
    
    // 实时读写速度（KB/s）
    uint32_t read_kbps;   // 读取速度
    uint32_t write_kbps;  // 写入速度
};

struct NasVolumeInfo {
    char name[32];
    uint32_t total_gb;
    uint32_t used_gb;
    uint8_t used_pct;
    char raid[16];
    char status[16];
};

struct NasServiceInfo {
    char name[32];
    bool running;
    bool is_docker;
};

#define MAX_NETWORK_INTERFACES 4  // 最多支持4个网口

struct NasNetworkInfo {
    char interface[16];
    char ip[16];
    uint32_t rx_bps;
    uint32_t tx_bps;
};

struct NasInterfaceInfo {
    char name[16];        // 网口名称 (eth0, eth1, wlan0等)
    char ip[16];          // IP地址
    uint32_t rx_bps;      // 下载速度
    uint32_t tx_bps;      // 上传速度
    bool active;          // 是否活跃
};

struct FanStatus {
    uint16_t rpm;
    uint8_t  pwm_pct;
    int16_t  ctrl_temp;
    bool     stall_alarm;
    bool     enabled;
};

struct NasData {
    NasSystemInfo   system;
    NasDiskInfo     disks[MAX_DISKS];
    uint8_t         disk_count;
    NasVolumeInfo   volumes[MAX_VOLUMES];
    uint8_t         volume_count;
    NasServiceInfo  services[MAX_SERVICES];
    uint8_t         service_count;
    NasNetworkInfo  network;  // 向后兼容，保留当前活动网口
    NasInterfaceInfo interfaces[MAX_NETWORK_INTERFACES];  // 所有网口信息
    uint8_t         interface_count;  // 网口数量
    uint8_t         active_interface_idx;  // 当前活动网口索引
    FanStatus       fan;
    uint32_t        last_update_ms;
    bool            is_online;
    bool            has_update;
};

// Fan curve configuration
struct FanCurvePoint {
    int16_t temp;
    uint8_t pwm_pct;
};

struct FanConfig {
    FanCurvePoint curve[FAN_CURVE_POINTS];
    TempSource temp_source;
    uint8_t hysteresis;
    uint8_t min_change_pct;
    uint8_t min_pwm_pct;
    int16_t emergency_temp;
    uint8_t stall_detect_sec;
    uint16_t ramp_time_ms;
    FanMode mode;
    uint8_t manual_pwm_pct;
    bool enabled;
};

// Default fan curve (quiet first)
static const FanCurvePoint DEFAULT_FAN_CURVE[FAN_CURVE_POINTS] = {
    { 25,  25 },
    { 35,  30 },
    { 45,  50 },
    { 55,  80 },
    { 65, 100 },
};
