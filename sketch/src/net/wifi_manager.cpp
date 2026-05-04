// ZotLab NAS Monitor - WiFi Manager Implementation
#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>
#include <esp_wifi.h>

WiFiManager g_wifi;

// ============================================================================
// WiFi 事件回调函数 - 实时打印连接状态变化
// ============================================================================
void wifi_event_callback(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
        case WIFI_EVENT_STA_START:
            Serial.println(F("[WiFi Event] STA Started"));
            break;
            
        case WIFI_EVENT_STA_CONNECTED:
            Serial.printf("[WiFi Event] STA Connected to SSID: %s\n", 
                         info.wifi_sta_connected.ssid);
            Serial.printf("[WiFi Event] BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
                         info.wifi_sta_connected.bssid[0],
                         info.wifi_sta_connected.bssid[1],
                         info.wifi_sta_connected.bssid[2],
                         info.wifi_sta_connected.bssid[3],
                         info.wifi_sta_connected.bssid[4],
                         info.wifi_sta_connected.bssid[5]);
            Serial.printf("[WiFi Event] Channel: %d, Auth: %d\n",
                         info.wifi_sta_connected.channel,
                         info.wifi_sta_connected.authmode);
            break;
            
        case WIFI_EVENT_STA_DISCONNECTED: {
            const char* reason_str = "UNKNOWN";
            switch (info.wifi_sta_disconnected.reason) {
                case WIFI_REASON_AUTH_EXPIRE: reason_str = "AUTH_EXPIRE"; break;
                case WIFI_REASON_AUTH_FAIL: reason_str = "AUTH_FAIL"; break;
                case WIFI_REASON_NO_AP_FOUND: reason_str = "NO_AP_FOUND"; break;
                case WIFI_REASON_CONNECTION_FAIL: reason_str = "CONNECTION_FAIL"; break;
                case WIFI_REASON_HANDSHAKE_TIMEOUT: reason_str = "HANDSHAKE_TIMEOUT"; break;
                case WIFI_REASON_ASSOC_EXPIRE: reason_str = "ASSOC_EXPIRE"; break;
                case WIFI_REASON_ASSOC_FAIL: reason_str = "ASSOC_FAIL"; break;
                case WIFI_REASON_NOT_AUTHED: reason_str = "NOT_AUTHED"; break;
                case WIFI_REASON_NOT_ASSOCED: reason_str = "NOT_ASSOCED"; break;
                case WIFI_REASON_ASSOC_LEAVE: reason_str = "ASSOC_LEAVE"; break;
                case WIFI_REASON_ASSOC_NOT_AUTHED: reason_str = "ASSOC_NOT_AUTHED"; break;
                case WIFI_REASON_DISASSOC_PWRCAP_BAD: reason_str = "DISASSOC_PWRCAP_BAD"; break;
                case WIFI_REASON_DISASSOC_SUPCHAN_BAD: reason_str = "DISASSOC_SUPCHAN_BAD"; break;
                case WIFI_REASON_IE_INVALID: reason_str = "IE_INVALID"; break;
                case WIFI_REASON_MIC_FAILURE: reason_str = "MIC_FAILURE"; break;
                case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT: reason_str = "4WAY_HANDSHAKE_TIMEOUT"; break;
                case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT: reason_str = "GROUP_KEY_UPDATE_TIMEOUT"; break;
                case WIFI_REASON_IE_IN_4WAY_DIFFERS: reason_str = "IE_IN_4WAY_DIFFERS"; break;
                case WIFI_REASON_GROUP_CIPHER_INVALID: reason_str = "GROUP_CIPHER_INVALID"; break;
                case WIFI_REASON_PAIRWISE_CIPHER_INVALID: reason_str = "PAIRWISE_CIPHER_INVALID"; break;
                case WIFI_REASON_AKMP_INVALID: reason_str = "AKMP_INVALID"; break;
                case WIFI_REASON_UNSUPP_RSN_IE_VERSION: reason_str = "UNSUPP_RSN_IE_VERSION"; break;
                case WIFI_REASON_INVALID_RSN_IE_CAP: reason_str = "INVALID_RSN_IE_CAP"; break;
                case WIFI_REASON_802_1X_AUTH_FAILED: reason_str = "802_1X_AUTH_FAILED"; break;
                case WIFI_REASON_CIPHER_SUITE_REJECTED: reason_str = "CIPHER_SUITE_REJECTED"; break;
                case WIFI_REASON_BEACON_TIMEOUT: reason_str = "BEACON_TIMEOUT"; break;
                case WIFI_REASON_UNSPECIFIED: reason_str = "UNSPECIFIED"; break;
                default: break;
            }
            Serial.printf("[WiFi Event] STA Disconnected! Reason: %d - %s\n", 
                         info.wifi_sta_disconnected.reason, reason_str);
            break;
        }
            
        case WIFI_EVENT_STA_AUTHMODE_CHANGE:
            Serial.printf("[WiFi Event] Auth mode changed: %d -> %d\n",
                         info.wifi_sta_authmode_change.old_mode,
                         info.wifi_sta_authmode_change.new_mode);
            break;
            
        case IP_EVENT_STA_GOT_IP:
            Serial.printf("[WiFi Event] Got IP: %s\n", 
                         IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str());
            Serial.printf("[WiFi Event] Gateway: %s\n",
                         IPAddress(info.got_ip.ip_info.gw.addr).toString().c_str());
            Serial.printf("[WiFi Event] Netmask: %s\n",
                         IPAddress(info.got_ip.ip_info.netmask.addr).toString().c_str());
            break;
            
        case IP_EVENT_STA_LOST_IP:
            Serial.println(F("[WiFi Event] Lost IP"));
            break;
            
        default:
            Serial.printf("[WiFi Event] Unknown event: %d\n", event);
            break;
    }
}

// 全局辅助函数:获取WiFiManager单例的RTC指针
ESP32Time* wifi_get_rtc() {
    return g_wifi.getRtc();
}

WiFiManager::WiFiManager()
    : state(WIFI_DISCONNECTED), last_reconnect_ms(0), connect_start_ms(0),
      connect_timeout_ms(15000), connecting_nonblocking(false), reconnect_attempts(0), network_count(0) {
    ip_str[0] = '\0';
    
    // 注册 WiFi 事件回调
    WiFi.onEvent(wifi_event_callback);
    Serial.println(F("[WiFi] Event callback registered"));
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

    // 检查当前状态，避免重复连接
    if (state == WIFI_CONNECTING) {
        Serial.println(F("[WiFi] Already connecting, skipping duplicate connect"));
        return;
    }

    Serial.printf("[WiFi] Starting non-blocking connection to SSID: '%s'\n", ssid);
    Serial.printf("[WiFi] Password length: %d characters\n", (password ? strlen(password) : 0));
    Serial.printf("[WiFi] Current WiFi status before begin: %d\n", WiFi.status());
    
    state = WIFI_CONNECTING;
    connecting_nonblocking = true;
    connect_start_ms = millis();

    // 如果已连接，先断开
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("[WiFi] Disconnecting from current network..."));
        WiFi.disconnect(true);  // 参数 true = 关闭 STA 模式
        delay(500);  // 增加等待时间
        Serial.printf("[WiFi] WiFi status after disconnect: %d\n", WiFi.status());
    }

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    
    Serial.println(F("[WiFi] Calling WiFi.begin()..."));
    WiFi.begin(ssid, password);
    
    // 立即检查状态
    Serial.printf("[WiFi] WiFi status immediately after begin: %d\n", WiFi.status());
    
    Serial.println(F("[WiFi] Non-blocking connection started, will check progress with checkConnectionProgress()"));
}

// 检查非阻塞连接进度（返回true表示完成）
bool WiFiManager::checkConnectionProgress() {
    if (!connecting_nonblocking) {
        return true;  // 不在连接中，视为完成
    }

    // 调试连接状态
    static uint32_t last_debug_ms = 0;
    if (millis() - last_debug_ms > 1000) {
        last_debug_ms = millis();
        int status = WiFi.status();
        const char* status_str = "UNKNOWN";
        switch (status) {
            case WL_IDLE_STATUS: status_str = "IDLE"; break;
            case WL_NO_SSID_AVAIL: status_str = "NO_SSID_AVAIL"; break;
            case WL_SCAN_COMPLETED: status_str = "SCAN_COMPLETED"; break;
            case WL_CONNECTED: status_str = "CONNECTED"; break;
            case WL_CONNECT_FAILED: status_str = "CONNECT_FAILED"; break;
            case WL_CONNECTION_LOST: status_str = "CONNECTION_LOST"; break;
            case WL_DISCONNECTED: status_str = "DISCONNECTED"; break;
        }
        Serial.printf("[WiFi] Connection status: %d - %s (elapsed: %ums)\n", 
                      status, status_str, millis() - connect_start_ms);
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
        int status = WiFi.status();
        const char* status_str = "UNKNOWN";
        switch (status) {
            case WL_IDLE_STATUS: status_str = "IDLE"; break;
            case WL_NO_SSID_AVAIL: status_str = "NO_SSID_AVAIL"; break;
            case WL_SCAN_COMPLETED: status_str = "SCAN_COMPLETED"; break;
            case WL_CONNECTED: status_str = "CONNECTED"; break;
            case WL_CONNECT_FAILED: status_str = "CONNECT_FAILED"; break;
            case WL_CONNECTION_LOST: status_str = "CONNECTION_LOST"; break;
            case WL_DISCONNECTED: status_str = "DISCONNECTED"; break;
        }
        Serial.printf("\n[WiFi] Non-blocking connection timeout! Status: %s\n", status_str);
        return true;  // 超时也视为完成（失败）
    }

    // 还在连接中
    return false;
}

void WiFiManager::check() {
    // 首先检查非阻塞连接进度（如果正在连接中）
    if (connecting_nonblocking) {
        bool done = checkConnectionProgress();
        if (done) {
            // 连接完成（成功或失败）
            Serial.printf("[WiFi] Non-blocking connection %s\n", 
                          (state == WIFI_CONNECTED) ? "SUCCESS" : "FAILED");
        }
        // 如果还在连接中，不执行重连逻辑
        if (state == WIFI_CONNECTING) {
            return;
        }
    }
    
    uint32_t now = millis();
    
    // 以下原有的重连逻辑...
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
    // 使用异步扫描模式，不断开当前连接
    // 参数: async=false, show_hidden=true, passive=false, max_ms=100
    network_count = WiFi.scanNetworks(false, true);
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
