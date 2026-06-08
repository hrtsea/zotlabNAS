// ZotLab NAS Monitor - NAS-specific configuration constants
#pragma once

#include <stdint.h>

// Display configuration (adapted for TuneBar ESP32-S3 172x640)
#ifndef TFT_HEIGHT
#define TFT_HEIGHT          172     // Portrait mode height
#endif

#ifndef TFT_WIDTH
#define TFT_WIDTH           640     // Portrait mode width
#endif

// ===== Application Info =====
#define APP_NAME           "ZotLab NAS Monitor"

#define APP_HOSTNAME       "NAS Monitor"
#define APP_SERIAL_PREFIX  "<< " APP_NAME " >>"

// ===== Splash Screen =====
#define NAS_LOGO           "ZotLab"
#define NAS_TYPE           "Z6"
#define APP_SUBTITLE       "System Monitor"

// ===== Version =====
#define APP_VERSION        "1.1.0"

// ===== Mock data mode (已移除，改为在 NAS_TYPES 中配置) =====
// Mock 现在作为一种普通的 NAS 类型，通过配置选择
// 使用方式: g_config.nas_type = "mock"

// ===== Default ports =====
#define DEFAULT_HTTP_PORT      8099   // nasmon.py HTTP server
#define DEFAULT_SYNOLOGY_PORT  5000   // DSM HTTP (5001 for HTTPS)
#define DEFAULT_QNAP_PORT     8080   // QTS HTTP
#define DEFAULT_TRUENAS_PORT   80     // TrueNAS HTTP
#define DEFAULT_NETDATA_PORT   19999  // Netdata REST API
#define DEFAULT_SNMP_PORT      161    // SNMP UDP
#define DEFAULT_SERIAL_BAUD    115200

// ===== Polling =====
#define DEFAULT_POLL_SEC       5      // default poll interval (seconds)
#define MIN_POLL_SEC           1
#define MAX_POLL_SEC           30

// ===== Serial protocol =====
#define SERIAL_STX             0x02   // Start of frame
#define SERIAL_ETX             0x03   // End of frame
#define SERIAL_BUF_SIZE        2048   // Frame buffer size
#define SERIAL_TIMEOUT_MS      3000   // Connection timeout

// ===== Data limits =====
#define MAX_DISKS              16     // 最大支持磁盘总数（数据结构上限）
#define MAX_VOLUMES            8
#define MAX_SERVICES           16
#define MAX_CPU_CORES          8
#define NET_HISTORY_POINTS     30     // Network chart rolling window

// ===== Default disk count (编译期默认值) =====
// 用户可以在系统设置中覆盖此值
#ifndef DEFAULT_SATA_DISK_COUNT
#define DEFAULT_SATA_DISK_COUNT     6      // 默认 SATA 硬盘数
#endif

#ifndef DEFAULT_M2_DISK_COUNT
#define DEFAULT_M2_DISK_COUNT       3      // 默认 M.2 硬盘数
#endif

// ===== Screen layout =====
#define STATUS_BAR_H           24     // Top status bar height
#define PAGE_DOT_H             8      // Bottom page indicator height
#define CONTENT_H              (TFT_HEIGHT - STATUS_BAR_H - PAGE_DOT_H) // 208px

#define TOTAL_PAGES            7      // Number of swipeable pages

// ===== Fan defaults =====
#define FAN_CURVE_POINTS       5
#define FAN_DEFAULT_HYSTERESIS 3      // degrees C
#define FAN_DEFAULT_MIN_PWM    20     // percent
#define FAN_DEFAULT_EMERGENCY  55     // degrees C
#define FAN_DEFAULT_RAMP_MS    2000   // soft start ramp time
#define FAN_DEFAULT_STALL_SEC  5      // stall detection timeout
#define FAN_ENABLED            true    // Default fan enabled

// ===== Status colors (RGB565) =====
#define COLOR_OK        0x07E0  // Green
#define COLOR_WARN      0xFFE0  // Yellow
#define COLOR_DANGER    0xF800  // Red
#define COLOR_INFO      0x07FF  // Cyan
#define COLOR_BG        0x0000  // Black
#define COLOR_TEXT      0xFFFF  // White
#define COLOR_DIM       0x4208  // Dark gray
