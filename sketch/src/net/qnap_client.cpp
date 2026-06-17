#include "qnap_client.h"
#include "data/config.h"
#include <WiFi.h>

QnapClient::QnapClient() {
    memset(&data, 0, sizeof(data));
    http = nullptr;
    sid[0] = '\0';
    username[0] = '\0';
    password[0] = '\0';
    state = QNAP_IDLE;
    consecutive_failures = 0;
}

QnapClient::~QnapClient() {
    if (http) { http->end(); delete http; }
}

bool QnapClient::init(Preferences& prefs) {
    prefs.getString(NVS_NAS_IP, nas_ip, sizeof(nas_ip));
    nas_port = prefs.getUShort(NVS_NAS_PORT, 8080);
    prefs.getString(NVS_NAS_USER, username, sizeof(username));
    prefs.getString(NVS_NAS_PASS, password, sizeof(password));
    http = new HTTPClient();
    http->setTimeout(3000);  // 3秒超时,避免阻塞UI
    Serial.printf("[QNAP] Init: %s:%d (user=%s)\n", nas_ip, nas_port, username);
    return strlen(nas_ip) > 0;
}

bool QnapClient::connect() {
    Serial.println("[QNAP] Connecting...");
    state = QNAP_LOGIN;
    bool success = login();
    if (success) {
        Serial.println("[QNAP] Connected successfully");
        data.is_online = true;
    } else {
        Serial.println("[QNAP] Connection failed, will retry in poll()");
        data.is_online = false;
        consecutive_failures = 1;
        state = QNAP_IDLE;
    }
    return success;
}

bool QnapClient::poll() {
    uint32_t now = millis();
    
    // 退避机制: 连续失败后延长轮询间隔
    uint32_t poll_interval = g_config.poll_sec * 1000UL;
    if (consecutive_failures > 0) {
        // 限制指数最大阶数,避免位移溢出(最多2^3=8倍)
        uint8_t capped = consecutive_failures;
        if (capped > 3) capped = 3;
        
        uint32_t backoff = poll_interval * (1u << capped);
        if (backoff > 60000) backoff = 60000;  // 钳制到60秒上限
        poll_interval = backoff;
    }
    
    if (now - last_poll_ms < poll_interval) return false;
    
    // 如果未连接,尝试重新登录
    if (!data.is_online) {
        if (login()) {
            Serial.println("[QNAP] Reconnected");
            data.is_online = true;
            consecutive_failures = 0;
            state = QNAP_FETCH_SYSINFO;
        } else {
            consecutive_failures++;
            Serial.printf("[QNAP] Reconnect failed (failures: %d, next retry in %ds)\n", 
                         consecutive_failures, poll_interval/1000);
            last_poll_ms = now;
            return false;
        }
    }
    
    bool success = false;
    
    switch (state) {
        case QNAP_IDLE:
            state = QNAP_FETCH_SYSINFO;
            // Fall through
            
        case QNAP_LOGIN:
            if (!login()) {
                data.is_online = false;
                consecutive_failures++;
                Serial.printf("[QNAP] Login failed (consecutive: %d)\n", consecutive_failures);
                last_poll_ms = now;
                return false;
            }
            Serial.println("[QNAP] Logged in");
            state = QNAP_FETCH_SYSINFO;
            // Fall through
            
        case QNAP_FETCH_SYSINFO:
            success = fetchSysInfo();
            if (success) {
                state = QNAP_FETCH_STORAGE;
            } else {
                data.is_online = false;
                consecutive_failures++;
            }
            break;
            
        case QNAP_FETCH_STORAGE:
            success = fetchStorage();
            if (success) {
                state = QNAP_FETCH_DISKS;
            } else {
                consecutive_failures++;
            }
            break;
            
        case QNAP_FETCH_DISKS:
            success = fetchDisks();
            if (success) {
                state = QNAP_FETCH_NETWORK;
            } else {
                consecutive_failures++;
            }
            break;
            
        case QNAP_FETCH_NETWORK:
            success = fetchNetwork();
            if (success) {
                // 成功后重置失败计数
                if (consecutive_failures > 0) {
                    Serial.printf("[QNAP] Connection recovered after %d failures\n", consecutive_failures);
                    consecutive_failures = 0;
                }
                data.is_online = true;
                data.last_update_ms = now;
                data.has_update = true;
                state = QNAP_DONE;
                Serial.println("[QNAP] Poll complete");
            } else {
                consecutive_failures++;
            }
            break;
            
        case QNAP_DONE:
            state = QNAP_FETCH_SYSINFO;
            last_poll_ms = now;
            return false;
    }
    
    last_poll_ms = now;
    return data.has_update;
}

bool QnapClient::login() {
    if (!WiFi.isConnected()) {
        Serial.println("[QNAP] WiFi not connected, skip login");
        return false;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d/cgi-bin/authLogin.cgi", nas_ip, nas_port);
    http->begin(url);
    http->addHeader("Content-Type", "application/x-www-form-urlencoded");
    char post[128];
    snprintf(post, sizeof(post), "user=%s&pwd=%s", username, password);
    int code = http->POST(post);
    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[2048];
        WiFiClient* stream = http->getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        const char* s = extractXML(resp, "authSid");
        if (s && strlen(s) > 0) {
            strlcpy(sid, s, sizeof(sid));
            http->end();
            return true;
        }
    }
    http->end();
    return false;
}

bool QnapClient::fetchSysInfo() {
    if (!WiFi.isConnected()) {
        data.is_online = false;
        return false;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d/cgi-bin/management/manaRequest.cgi?subfunc=sysinfo&hd=no&multicpu=1&_sid=%s",
             nas_ip, nas_port, sid);
    http->begin(url);
    int code = http->GET();
    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[4096];
        WiFiClient* stream = http->getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        const char* xml = resp;
        
        const char* cpu = extractXML(xml, "cpuusage");
        if (cpu) data.system.cpu_pct = atof(cpu);
        
        const char* mem = extractXML(xml, "mem_perc");
        if (mem) data.system.ram_pct = atof(mem);
        
        const char* temp = extractXML(xml, "cpu_temp");
        if (temp) data.system.temp_cpu = atoi(temp);
        
        const char* mem_total = extractXML(xml, "mem_total");
        if (mem_total) data.system.ram_total_mb = atoi(mem_total);
        
        http->end();
        return true;
    }
    http->end();
    return false;
}

bool QnapClient::fetchStorage() {
    if (!WiFi.isConnected()) {
        data.is_online = false;
        return false;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d/cgi-bin/management/chartReq.cgi?chart_func=disk_usage&disk_select=all&_sid=%s",
             nas_ip, nas_port, sid);
    http->begin(url);
    int code = http->GET();
    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[1024];  // 大幅减少
        WiFiClient* stream = http->getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        const char* xml = resp;
        
        // Parse volume information
        // QNAP returns multiple <volume> blocks
        for (int i = 0; i < MAX_VOLUMES; i++) {
            char idx_str[8];
            snprintf(idx_str, sizeof(idx_str), "%d", i);
            
            // Try to find volume by index
            const char* name = extractXML(xml, "volume_name", i);
            if (!name) break;
            
            strlcpy(data.volumes[i].name, name, sizeof(data.volumes[i].name));
            
            const char* total = extractXML(xml, "total_size", i);
            if (total) data.volumes[i].total_gb = atoi(total) / 1024; // MB to GB
            
            const char* used = extractXML(xml, "used_size", i);
            if (used) data.volumes[i].used_gb = atoi(used) / 1024;
            
            if (data.volumes[i].total_gb > 0) {
                data.volumes[i].used_pct = (data.volumes[i].used_gb * 100) / data.volumes[i].total_gb;
            }
            
            const char* raid = extractXML(xml, "raid_type", i);
            if (raid) strlcpy(data.volumes[i].raid, raid, sizeof(data.volumes[i].raid));
            
            const char* status = extractXML(xml, "volume_status", i);
            if (status) strlcpy(data.volumes[i].status, status, sizeof(data.volumes[i].status));
            
            data.volume_count++;
        }
        
        http->end();
        return true;
    }
    http->end();
    return false;
}

bool QnapClient::fetchDisks() {
    if (!WiFi.isConnected()) {
        data.is_online = false;
        return false;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d/cgi-bin/disk/qsmart.cgi?func=all_hd_data&_sid=%s",
             nas_ip, nas_port, sid);
    http->begin(url);
    int code = http->GET();
    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[2048];  // 大幅减少
        WiFiClient* stream = http->getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        const char* xml = resp;
        
        // Parse disk information
        for (int i = 0; i < MAX_DISKS; i++) {
            const char* name = extractXML(xml, "disk_name", i);
            if (!name) break;
            
            strlcpy(data.disks[i].name, name, sizeof(data.disks[i].name));
            
            const char* model = extractXML(xml, "disk_model", i);
            if (model) strlcpy(data.disks[i].model_name, model, sizeof(data.disks[i].model_name));
            
            const char* temp = extractXML(xml, "disk_temp", i);
            if (temp) data.disks[i].temp = atoi(temp);
            
            const char* health = extractXML(xml, "disk_health", i);
            if (health) {
                if (strstr(health, "Good") || strstr(health, "Normal")) {
                    data.disks[i].health = HEALTH_OK;
                } else if (strstr(health, "Warning")) {
                    data.disks[i].health = HEALTH_WARNING;
                } else {
                    data.disks[i].health = HEALTH_CRITICAL;
                }
            }
            
            const char* size = extractXML(xml, "disk_size", i);
            if (size) data.disks[i].size_gb = atoi(size);
            
            data.disk_count++;
        }
        
        http->end();
        return true;
    }
    http->end();
    return false;
}

bool QnapClient::fetchNetwork() {
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d/cgi-bin/management/chartReq.cgi?chart_func=QSM40bandwidth&_sid=%s",
             nas_ip, nas_port, sid);
    http->begin(url);
    int code = http->GET();
    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[512];  // 大幅减少
        WiFiClient* stream = http->getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        const char* xml = resp;
        
        const char* rx = extractXML(xml, "net_rx");
        if (rx) data.network.rx_bps = (uint32_t)(atof(rx) * 1000 / 8); // Kbits/s to bytes/s
        
        const char* tx = extractXML(xml, "net_tx");
        if (tx) data.network.tx_bps = (uint32_t)(atof(tx) * 1000 / 8);
        
        const char* iface = extractXML(xml, "interface");
        if (iface) strlcpy(data.network.interface, iface, sizeof(data.network.interface));
        
        const char* ip = extractXML(xml, "ip");
        if (ip) strlcpy(data.network.ip, ip, sizeof(data.network.ip));
        
        http->end();
        return true;
    }
    http->end();
    return false;
}

bool QnapClient::isConnected() { return data.is_online; }
const NasData& QnapClient::getData() { return data; }
const char* QnapClient::getTypeName() { return "QNAP QTS"; }
const char* QnapClient::getConnIcon() { return "wifi"; }

const char* QnapClient::extractXML(const char* xml, const char* tag, int index) {
    static char buf[128];
    char start[64], end_[64];
    snprintf(start, sizeof(start), "<%s>", tag);
    snprintf(end_, sizeof(end_), "</%s>", tag);
    
    const char* p = xml;
    for (int i = 0; i <= index; i++) {
        p = strstr(p, start);
        if (!p) return nullptr;
        if (i < index) p += strlen(start);
    }
    
    p += strlen(start);
    const char* p2 = strstr(p, end_);
    if (!p2) return nullptr;
    
    size_t len = min((size_t)(p2 - p), sizeof(buf) - 1);
    strncpy(buf, p, len);
    buf[len] = '\0';
    return buf;
}
