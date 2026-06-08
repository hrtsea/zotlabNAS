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
            
            // 物理连接成功，但尚未获得 IP
            g_wifi.onStaConnected();
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
            // Update WiFiManager state to match actual hardware state
            g_wifi.onStaDisconnected();
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
            
            // 关键：当成功获取 IP 时，状态变为 CONNECTED
            g_wifi.onStaGotIP(IPAddress(info.got_ip.ip_info.ip.addr).toString());
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
      connect_timeout_ms(25000), connecting_nonblocking(false), reconnect_attempts(0), network_count(0) {
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

    // 强制重置连接状态，防止之前卡死的状态阻塞新连接
    if (connecting_nonblocking && WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[WiFi] Resetting stale connection state"));
        connecting_nonblocking = false;
    }

    // 检查当前状态，避免重复连接
    if (state == WIFI_CONNECTING && connecting_nonblocking) {
        Serial.println(F("[WiFi] Already connecting, skipping duplicate connect"));
        return;
    }

    Serial.printf("[WiFi] Starting non-blocking connection to SSID: '%s'\n", ssid);
    if (password && strlen(password) > 0) {
        Serial.printf("[WiFi] Password configured (length: %d)\n", strlen(password));
    } else {
        Serial.println(F("[WiFi] No password (open network or WPS?)"));
    }
    Serial.printf("[WiFi] Current WiFi status before begin: %d\n", WiFi.status());
    
    // 扫描网络，检查目标SSID是否存在以及加密方式
    Serial.println(F("[WiFi] Scanning networks to check target SSID..."));
    int n = WiFi.scanNetworks(false, true);  // async=false, show_hidden=true
    bool target_found = false;
    for (int i = 0; i < n; i++) {
        String found_ssid = WiFi.SSID(i);
        if (found_ssid == ssid) {
            target_found = true;
            Serial.printf("[WiFi] Target SSID found! RSSI: %d dBm, Encryption: %d\n",
                          WiFi.RSSI(i), WiFi.encryptionType(i));
            // 打印加密方式
            const char* enc_str = "UNKNOWN";
            switch (WiFi.encryptionType(i)) {
                case WIFI_AUTH_OPEN: enc_str = "OPEN"; break;
                case WIFI_AUTH_WEP: enc_str = "WEP"; break;
                case WIFI_AUTH_WPA_PSK: enc_str = "WPA_PSK"; break;
                case WIFI_AUTH_WPA2_PSK: enc_str = "WPA2_PSK"; break;
                case WIFI_AUTH_WPA_WPA2_PSK: enc_str = "WPA_WPA2_PSK"; break;
                case WIFI_AUTH_WPA2_ENTERPRISE: enc_str = "WPA2_ENTERPRISE"; break;
                default: break;
            }
            Serial.printf("[WiFi] Encryption type: %s\n", enc_str);
            break;
        }
    }
    if (!target_found) {
        Serial.printf("[WiFi] WARNING: Target SSID '%s' NOT FOUND in scan!\n", ssid);
        Serial.println(F("[WiFi] Possible causes: 1) Wrong SSID 2) Router not broadcasting SSID 3) Out of range"));
    }
    WiFi.scanDelete();  // 释放扫描结果
    
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
    
    // 打印 WiFi 配置信息
    Serial.printf("[WiFi] [DEBUG] Mode: STA | AutoReconnect: %s | SSID: %s (%d chars)\n",
                  WiFi.getAutoReconnect() ? "ON" : "OFF", ssid, strlen(ssid));
    Serial.printf("[WiFi] [DEBUG] Password: %s (%d chars)\n", 
                  password ? "********" : "NONE", password ? strlen(password) : 0);
    
    Serial.println(F("[WiFi] Calling WiFi.begin()..."));
    WiFi.begin(ssid, password);
    
    // 立即检查状态
    int status_after_begin = WiFi.status();
    Serial.printf("[WiFi] [DEBUG] Status after begin: %d\n", status_after_begin);
    
    if (status_after_begin == WL_CONNECT_FAILED) {
        Serial.println(F("[WiFi] ERROR: WiFi.begin() failed immediately! Check SSID/password"));
    }
    
    Serial.println(F("[WiFi] Non-blocking connection started, will check progress with checkConnectionProgress()"));
}

// 检查非阻塞连接进度（返回true表示完成）
bool WiFiManager::checkConnectionProgress() {
    if (!connecting_nonblocking) {
        return true;  // 不在连接中，视为完成
    }

    // 详细的调试连接状态（每 500ms）
    static uint32_t last_debug_ms = 0;
    if (millis() - last_debug_ms > 500) {
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
        
        // 额外的连接信息
        int8_t rssi = WiFi.RSSI();
        Serial.printf("[WiFi] [DEBUG] Status: %d-%s | RSSI: %ddBm | Elapsed: %ums | Target: %s\n", 
                      status, status_str, rssi, millis() - connect_start_ms, g_config.ssid);
        
        // 显示物理连接但IP获取中的状态
        if (status != WL_CONNECTED && state == WIFI_CONNECTING) {
            String bssid = WiFi.BSSIDstr();
            int channel = WiFi.channel();
            Serial.printf("[WiFi] [DEBUG] - Physical: BSSID=%s | Channel=%d | RSSI=%ddBm\n", 
                          bssid.c_str(), channel, rssi);
            if (millis() - connect_start_ms > 15000) {
                Serial.printf("[WiFi] [DEBUG] - DHCP waiting... IP acquisition slow\n");
            }
        }
    }

    // 检查是否连接成功
    if (WiFi.status() == WL_CONNECTED) {
        // 确保与事件回调同步
        if (state != WIFI_CONNECTED) {
            state = WIFI_CONNECTED;
            connecting_nonblocking = false;
            snprintf(ip_str, sizeof(ip_str), "%s", WiFi.localIP().toString().c_str());
            Serial.printf("\n[WiFi] [DEBUG] Non-blocking connected! IP: %s\n", ip_str);
            Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
            Serial.printf("[WiFi] [DEBUG] State synced: CONNECTING -> CONNECTED (WiFi.status() check)\n");
        }
        return true;
    }

    // 检查是否超时 - 更宽松的DHCP超时判断
    if (millis() - connect_start_ms >= connect_timeout_ms) {
        // 检查WiFi底层状态，可能已物理连接但IP获取慢
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
        
        // 如果已物理连接但无IP，可能是DHCP慢，不要立即断开
        if (status == WL_IDLE_STATUS || status == WL_DISCONNECTED) {
            state = WIFI_DISCONNECTED;
            connecting_nonblocking = false;
            Serial.printf("\n[WiFi] [DEBUG] Connection TIMEOUT! Status: %s\n", status_str);
            Serial.printf("[WiFi] [DEBUG] - Connect attempt duration: %ums\n", millis() - connect_start_ms);
            Serial.printf("[WiFi] [DEBUG] - Target: SSID='%s', Pass=%d chars\n", g_config.ssid, strlen(g_config.wifipass));
            Serial.printf("[WiFi] [DEBUG] - Possible causes: Password wrong, AP not available, signal weak\n");
            return true;
        } else {
            // 物理连接已建立但IP获取慢，继续等待
            Serial.printf("\n[WiFi] [DEBUG] DHCP waiting... Status: %s | Elapsed: %ums\n", 
                          status_str, millis() - connect_start_ms);
            Serial.printf("[WiFi] [DEBUG] - Physical connected but no IP yet\n");
            return false;  // 不视为完成，继续等待
        }
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

void WiFiManager::disconnect(bool stopWiFi) {
    WiFi.disconnect(stopWiFi);
    state = WIFI_DISCONNECTED;
    connecting_nonblocking = false;
    reconnect_attempts = 0;
    Serial.println(F("[WiFi] Disconnected and state reset"));
}

void WiFiManager::onStaConnected() {
    // Only update if we thought we were disconnected
    if (state == WIFI_DISCONNECTED) {
        Serial.printf("[WiFi] [DEBUG] State synced: DISCONNECTED -> CONNECTING\n");
        Serial.printf("[WiFi] [DEBUG] - Physical connection established\n");
        
        // 获取物理连接信息
        int rssi = WiFi.RSSI();
        int channel = WiFi.channel();
        Serial.printf("[WiFi] [DEBUG] - RSSI: %ddBm | Channel: %d\n", rssi, channel);
    }
    state = WIFI_CONNECTING;
    connecting_nonblocking = true;
}

void WiFiManager::onStaGotIP(const String& ip_addr) {
    state = WIFI_CONNECTED;
    connecting_nonblocking = false;
    strncpy(ip_str, ip_addr.c_str(), sizeof(ip_str) - 1);
    ip_str[sizeof(ip_str) - 1] = '\0';
    
    // 获取详细的网络信息
    String gateway = WiFi.gatewayIP().toString();
    String subnet = WiFi.subnetMask().toString();
    String dns1 = WiFi.dnsIP(0).toString();
    String dns2 = WiFi.dnsIP(1).toString();
    int rssi = WiFi.RSSI();
    
    Serial.printf("[WiFi] [DEBUG] State synced: CONNECTING -> CONNECTED\n");
    Serial.printf("[WiFi] [DEBUG] - IP Address: %s\n", ip_str);
    Serial.printf("[WiFi] [DEBUG] - Gateway: %s\n", gateway.c_str());
    Serial.printf("[WiFi] [DEBUG] - Subnet Mask: %s\n", subnet.c_str());
    Serial.printf("[WiFi] [DEBUG] - DNS1: %s | DNS2: %s\n", dns1.c_str(), dns2.c_str());
    Serial.printf("[WiFi] [DEBUG] - RSSI: %ddBm | Signal: %s\n", 
                  rssi, (rssi > -60 ? "Strong" : rssi > -70 ? "Good" : "Weak"));
    Serial.printf("[WiFi] [DEBUG] - Connection duration: %ums\n", millis() - connect_start_ms);
}

void WiFiManager::onStaDisconnected() {
    // Only update if we thought we were connected or connecting
    if (state == WIFI_CONNECTED || state == WIFI_CONNECTING || state == WIFI_RECONNECTING) {
        Serial.printf("[WiFi] [DEBUG] State synced: %d -> DISCONNECTED\n", state);
    }
    state = WIFI_DISCONNECTED;
    connecting_nonblocking = false;
}
