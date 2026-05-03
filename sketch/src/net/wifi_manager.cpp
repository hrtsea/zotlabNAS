// ZotLab NAS Monitor - WiFi Manager Implementation
#include "wifi_manager.h"
#include "config.h"

WiFiManager g_wifi;

// 全局辅助函数:获取WiFiManager单例的RTC指针
ESP32Time* wifi_get_rtc() {
    return g_wifi.getRtc();
}

WiFiManager::WiFiManager()
    : state(WIFI_DISCONNECTED), last_reconnect_ms(0), connect_start_ms(0),
      connect_timeout_ms(15000), connecting_nonblocking(false), reconnect_attempts(0), network_count(0) {
    ip_str[0] = '\0';
}

bool WiFiManager::connect(const char* ssid, const char* password, uint32_t timeout_ms) {
    if (!ssid || strlen(ssid) == 0) {
        Serial.println(F("[WiFi] No SSID configured"));
        state = WIFI_DISCONNECTED;
        return false;
    }

    Serial.printf("[WiFi] Connecting to %s...\n", ssid);
    state = WIFI_CONNECTING;

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(ssid, password);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeout_ms) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        state = WIFI_CONNECTED;
        snprintf(ip_str, sizeof(ip_str), "%s", WiFi.localIP().toString().c_str());
        Serial.printf("\n[WiFi] Connected! IP: %s\n", ip_str);
        Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
        return true;
    }

    Serial.println("\n[WiFi] Connection timeout");
    state = WIFI_DISCONNECTED;
    return false;
}

// 非阻塞WiFi连接启动
void WiFiManager::connectNonBlocking(const char* ssid, const char* password) {
    if (!ssid || strlen(ssid) == 0) {
        Serial.println(F("[WiFi] No SSID configured"));
        state = WIFI_DISCONNECTED;
        return;
    }

    Serial.printf("[WiFi] Starting non-blocking connection to %s...\n", ssid);
    state = WIFI_CONNECTING;
    connecting_nonblocking = true;
    connect_start_ms = millis();

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(ssid, password);
    
    Serial.println(F("[WiFi] Non-blocking connection started, check progress with checkConnectionProgress()"));
}

// 检查非阻塞连接进度（返回true表示完成）
bool WiFiManager::checkConnectionProgress() {
    if (!connecting_nonblocking) {
        return true;  // 不在连接中，视为完成
    }

    // 检查是否连接成功
    if (WiFi.status() == WL_CONNECTED) {
        state = WIFI_CONNECTED;
        connecting_nonblocking = false;
        snprintf(ip_str, sizeof(ip_str), "%s", WiFi.localIP().toString().c_str());
        Serial.printf("\n[WiFi] Non-blocking connected! IP: %s\n", ip_str);
        Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
        return true;
    }

    // 检查是否超时
    if (millis() - connect_start_ms >= connect_timeout_ms) {
        state = WIFI_DISCONNECTED;
        connecting_nonblocking = false;
        Serial.println("\n[WiFi] Non-blocking connection timeout");
        return true;  // 超时也视为完成（失败）
    }

    // 还在连接中
    return false;
}

void WiFiManager::check() {
    uint32_t now = millis();

    if (WiFi.status() == WL_CONNECTED) {
        if (state != WIFI_CONNECTED) {
            state = WIFI_CONNECTED;
            snprintf(ip_str, sizeof(ip_str), "%s", WiFi.localIP().toString().c_str());
            Serial.printf("[WiFi] Reconnected! IP: %s\n", ip_str);
        }
        // 成功后重置重连计数
        if (reconnect_attempts > 0) {
            Serial.printf("[WiFi] Connection stable, resetting reconnect counter\n");
            reconnect_attempts = 0;
        }
        last_reconnect_ms = now;
        return;
    }

    // Not connected
    if (state != WIFI_DISCONNECTED && state != WIFI_RECONNECTING) {
        state = WIFI_RECONNECTING;
        Serial.println(F("[WiFi] Disconnected, will attempt reconnect..."));
    }

    // 检查是否超过最大重连次数
    if (reconnect_attempts >= MAX_RECONNECT_ATTEMPTS) {
        // 进入休眠模式，不再尝试重连
        if (state != WIFI_DISCONNECTED) {
            Serial.printf("[WiFi] Max reconnect attempts (%d) reached, entering sleep mode\n",
                         MAX_RECONNECT_ATTEMPTS);
            state = WIFI_DISCONNECTED;
        }
        return;
    }

    // 计算下次重连间隔（指数退避）
    uint32_t next_interval = getNextReconnectInterval();
    
    // Attempt reconnect at intervals
    if (now - last_reconnect_ms >= next_interval) {
        last_reconnect_ms = now;
        if (strlen(g_config.ssid) > 0) {
            reconnect_attempts++;
            Serial.printf("[WiFi] Attempting reconnect %d/%d (interval: %ums)...\n",
                         reconnect_attempts, MAX_RECONNECT_ATTEMPTS, next_interval);
            WiFi.reconnect();
        }
    }
}

// 获取下次重连间隔（指数退避）
uint32_t WiFiManager::getNextReconnectInterval() {
    // 指数退避：10s → 20s → 40s → 80s → 160s → 320s → 封顶300s
    uint8_t capped = reconnect_attempts;
    if (capped > 5) capped = 5;  // 最多2^5=32倍
    
    uint32_t interval = BASE_RECONNECT_INTERVAL_MS * (1 << capped);
    if (interval > MAX_RECONNECT_INTERVAL_MS) {
        interval = MAX_RECONNECT_INTERVAL_MS;
    }
    return interval;
}

bool WiFiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

WifiState WiFiManager::getState() const {
    return state;
}

const char* WiFiManager::getIP() const {
    return ip_str;
}

int32_t WiFiManager::getRSSI() const {
    return WiFi.RSSI();
}

void WiFiManager::syncTime(int8_t timezone_offset) {
    configTime(timezone_offset * 3600, 0, "pool.ntp.org", "time.nist.gov");
    Serial.printf("[NTP] Time sync initiated (GMT+%d)\n", timezone_offset);
}

int WiFiManager::scanNetworks() {
    Serial.println(F("[WiFi] Scanning networks..."));
    WiFi.disconnect();
    delay(100);
    network_count = WiFi.scanNetworks();
    Serial.printf("[WiFi] Found %d networks\n", network_count);
    return network_count;
}

int WiFiManager::getNetworkCount() const {
    return network_count;
}

String WiFiManager::getSSID(int index) const {
    if (index >= 0 && index < network_count) {
        return WiFi.SSID(index);
    }
    return "";
}

int32_t WiFiManager::getNetworkRSSI(int index) const {
    if (index >= 0 && index < network_count) {
        return WiFi.RSSI(index);
    }
    return 0;
}

ESP32Time* WiFiManager::getRtc() {
    return &rtc;
}
