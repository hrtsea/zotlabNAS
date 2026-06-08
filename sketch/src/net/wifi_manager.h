// ZotLab NAS Monitor - WiFi Manager
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Time.h>

enum WifiState {
    WIFI_DISCONNECTED,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_RECONNECTING
};

class WiFiManager {
public:
    WiFiManager();

    // Connect to WiFi (blocking with timeout) - 保留向后兼容
    bool connect(const char* ssid, const char* password, uint32_t timeout_ms = 10000);
    
    // Non-blocking WiFi connection start
    void connectNonBlocking(const char* ssid, const char* password);
    
    // Check non-blocking connection progress (returns true when done)
    bool checkConnectionProgress();

    // Non-blocking WiFi status check (call from loop)
    void check();

    // Check if currently connected
    bool isConnected() const;

    // Get connection state
    WifiState getState() const;

    // Get IP address string
    const char* getIP() const;

    // Get WiFi signal strength (RSSI)
    int32_t getRSSI() const;

    // Sync time via NTP
    void syncTime(int8_t timezone_offset);

    // Scan for available networks
    int scanNetworks();

    // Get number of scanned networks
    int getNetworkCount() const;

    // Get SSID of scanned network at index
    String getSSID(int index) const;

    // Get RSSI of scanned network at index
    int32_t getNetworkRSSI(int index) const;

    // Get RTC pointer
    ESP32Time* getRtc();

    // Disconnect and reset state
    void disconnect(bool stopWiFi = false);

    // Handle STA disconnected event (called from event callback)
    void onStaDisconnected();
    
    // Handle STA connected event (called from event callback)
    void onStaConnected();
    
    // Handle STA got IP event (called from event callback)
    void onStaGotIP(const String& ip_addr);

private:
    WifiState state;
    ESP32Time rtc;
    char ip_str[16];
    uint32_t last_reconnect_ms;
    uint32_t connect_start_ms;  // 非阻塞连接开始时间
    uint32_t connect_timeout_ms;  // 连接超时时间
    bool connecting_nonblocking;  // 是否正在非阻塞连接
    uint8_t reconnect_attempts;  // 重连尝试次数
    int network_count;
    static const uint32_t BASE_RECONNECT_INTERVAL_MS = 10000;   // 基础10秒
    static const uint32_t MAX_RECONNECT_INTERVAL_MS = 300000;  // 最大5分钟
    static const uint8_t MAX_RECONNECT_ATTEMPTS = 10;          // 最多10次
    
    uint32_t getNextReconnectInterval();  // 计算重连间隔（指数退避）
};

extern WiFiManager g_wifi;

// Global helper function: get RTC pointer from WiFiManager
ESP32Time* wifi_get_rtc();
