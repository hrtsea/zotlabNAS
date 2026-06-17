#include "netdata_client.h"
#include "data/config.h"
#include <ArduinoJson.h>
#include <WiFi.h>

NetdataClient::NetdataClient() {
    memset(&data, 0, sizeof(data));
    http = nullptr;
    state = NETDATA_IDLE;
    consecutive_failures = 0;
}

NetdataClient::~NetdataClient() {
    if (http) { http->end(); delete http; }
}

bool NetdataClient::init(Preferences& prefs) {
    prefs.getString(NVS_NAS_IP, nas_ip, sizeof(nas_ip));
    nas_port = prefs.getUShort(NVS_NAS_PORT, 19999);
    http = new HTTPClient();
    http->setTimeout(3000);
    Serial.printf("[Netdata] Init: %s:%d\n", nas_ip, nas_port);
    return strlen(nas_ip) > 0;
}

bool NetdataClient::connect() {
    Serial.println("[Netdata] Connecting...");
    // 尝试获取CPU数据来验证连接
    if (fetchCPU()) {
        Serial.println("[Netdata] Connected successfully");
        data.is_online = true;
        state = NETDATA_IDLE;
        return true;
    } else {
        Serial.println("[Netdata] Connection failed, will retry in poll()");
        data.is_online = false;
        consecutive_failures = 1;
        state = NETDATA_IDLE;
        return false;
    }
}

bool NetdataClient::poll() {
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
    
    // 如果未连接,尝试重新连接
    if (!data.is_online) {
        if (fetchCPU()) {
            Serial.println("[Netdata] Reconnected");
            data.is_online = true;
            consecutive_failures = 0;
        } else {
            consecutive_failures++;
            Serial.printf("[Netdata] Reconnect failed (failures: %d, next retry in %ds)\n", 
                         consecutive_failures, poll_interval/1000);
            last_poll_ms = now;
            return false;
        }
    }
    
    bool success = false;
    
    switch (state) {
        case NETDATA_IDLE:
            state = NETDATA_FETCH_CPU;
            // Fall through
            
        case NETDATA_FETCH_CPU:
            success = fetchCPU();
            if (success) {
                state = NETDATA_FETCH_RAM;
            } else {
                data.is_online = false;
                consecutive_failures++;
            }
            break;
            
        case NETDATA_FETCH_RAM:
            success = fetchRAM();
            if (success) {
                state = NETDATA_FETCH_NETWORK;
            } else {
                consecutive_failures++;
            }
            break;
            
        case NETDATA_FETCH_NETWORK:
            success = fetchNetwork();
            if (success) {
                state = NETDATA_FETCH_STORAGE;
            } else {
                consecutive_failures++;
            }
            break;
            
        case NETDATA_FETCH_STORAGE:
            success = fetchStorage();
            if (success) {
                // 成功后重置失败计数
                if (consecutive_failures > 0) {
                    Serial.printf("[Netdata] Connection recovered after %d failures\n", consecutive_failures);
                    consecutive_failures = 0;
                }
                data.is_online = true;
                data.last_update_ms = now;
                data.has_update = true;
                state = NETDATA_DONE;
                Serial.println("[Netdata] Poll complete");
            } else {
                consecutive_failures++;
            }
            break;
            
        case NETDATA_DONE:
            state = NETDATA_FETCH_CPU;
            last_poll_ms = now;
            return false;
    }
    
    last_poll_ms = now;
    return data.has_update;
}

bool NetdataClient::fetchCPU() {
    if (!WiFi.isConnected()) {
        data.is_online = false;
        return false;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d/api/v1/data?chart=system.cpu&points=1&after=-1&format=json",
             nas_ip, nas_port);
    http->begin(url);
    int code = http->GET();
    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[512];  // 大幅减少
        WiFiClient* stream = http->getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, resp);
        http->end();
        if (!err) {
            JsonArray arr = doc["data"].as<JsonArray>();
            if (!arr.isNull() && arr.size() > 0) {
                JsonObject first = arr[0].as<JsonObject>();
                JsonObject dims = first["dimensions"].as<JsonObject>();
                float idle = dims["idle"].as<float>();
                data.system.cpu_pct = 100.0f - idle;
                Serial.printf("[Netdata] CPU: %.1f%%\n", data.system.cpu_pct);
            }
            return true;
        }
    }
    http->end();
    return false;
}

bool NetdataClient::fetchRAM() {
    if (!WiFi.isConnected()) {
        data.is_online = false;
        return false;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d/api/v1/data?chart=system.ram&points=1&after=-1&format=json",
             nas_ip, nas_port);
    http->begin(url);
    int code = http->GET();
    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[512];  // 大幅减少
        WiFiClient* stream = http->getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, resp);
        http->end();
        if (!err) {
            JsonArray arr = doc["data"].as<JsonArray>();
            if (!arr.isNull() && arr.size() > 0) {
                JsonObject first = arr[0].as<JsonObject>();
                JsonObject dims = first["dimensions"].as<JsonObject>();
                
                // Netdata returns bytes, convert to MB
                uint64_t used = dims["used"].as<uint64_t>();
                uint64_t total = dims["total"].as<uint64_t>();
                uint64_t cached = dims["cached"].as<uint64_t>();
                
                data.system.ram_used_mb = (uint32_t)(used / (1024 * 1024));
                data.system.ram_total_mb = (uint32_t)(total / (1024 * 1024));
                data.system.ram_cached_mb = (uint32_t)(cached / (1024 * 1024));
                data.system.ram_free_mb = data.system.ram_total_mb - data.system.ram_used_mb;
                
                if (data.system.ram_total_mb > 0) {
                    data.system.ram_pct = (float)data.system.ram_used_mb / data.system.ram_total_mb * 100.0f;
                }
                Serial.printf("[Netdata] RAM: %d/%d MB (%.1f%%)\n", 
                              data.system.ram_used_mb, data.system.ram_total_mb, data.system.ram_pct);
            }
            return true;
        }
    }
    http->end();
    return false;
}

bool NetdataClient::fetchNetwork() {
    if (!WiFi.isConnected()) {
        data.is_online = false;
        return false;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d/api/v1/data?chart=system.net&points=1&after=-1&format=json",
             nas_ip, nas_port);
    http->begin(url);
    int code = http->GET();
    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[512];  // 大幅减少
        WiFiClient* stream = http->getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, resp);
        http->end();
        if (!err) {
            JsonArray arr = doc["data"].as<JsonArray>();
            if (!arr.isNull() && arr.size() > 0) {
                JsonObject first = arr[0].as<JsonObject>();
                JsonArray dims = first["dimensions"].as<JsonArray>();
                if (!dims.isNull() && dims.size() >= 2) {
                    // Netdata system.net returns kilobits/s
                    // dimensions is an array of values: [received, sent]
                    float rx_kbps = dims[0].as<float>();
                    float tx_kbps = dims[1].as<float>();
                    
                    // Convert kilobits/s to bytes/s
                    data.network.rx_bps = (uint32_t)(rx_kbps * 1000.0f / 8.0f);
                    data.network.tx_bps = (uint32_t)(tx_kbps * 1000.0f / 8.0f);
                    
                    strlcpy(data.network.interface, "eth0", sizeof(data.network.interface));
                    Serial.printf("[Netdata] Net: RX=%.1f KB/s, TX=%.1f KB/s\n",
                                  data.network.rx_bps / 1024.0f, data.network.tx_bps / 1024.0f);
                }
            }
            return true;
        }
    }
    http->end();
    return false;
}

bool NetdataClient::fetchStorage() {
    if (!WiFi.isConnected()) {
        data.is_online = false;
        return false;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d/api/v1/data?chart=disk_space._&points=1&after=-1&format=json",
             nas_ip, nas_port);
    http->begin(url);
    int code = http->GET();
    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[512];  // 大幅减少
        WiFiClient* stream = http->getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, resp);
        http->end();
        if (!err) {
            JsonArray arr = doc["data"].as<JsonArray>();
            if (!arr.isNull() && arr.size() > 0) {
                JsonObject first = arr[0].as<JsonObject>();
                JsonObject dims = first["dimensions"].as<JsonObject>();
                
                // disk_space returns bytes
                uint64_t avail = dims["avail"].as<uint64_t>();
                uint64_t used = dims["used"].as<uint64_t>();
                
                uint64_t total = avail + used;
                if (total > 0) {
                    data.volumes[0].total_gb = (uint32_t)(total / (1024ULL * 1024ULL * 1024ULL));
                    data.volumes[0].used_gb = (uint32_t)(used / (1024ULL * 1024ULL * 1024ULL));
                    data.volumes[0].used_pct = (uint8_t)((used * 100) / total);
                    strlcpy(data.volumes[0].name, "/", sizeof(data.volumes[0].name));
                    strlcpy(data.volumes[0].raid, "none", sizeof(data.volumes[0].raid));
                    strlcpy(data.volumes[0].status, "normal", sizeof(data.volumes[0].status));
                    data.volume_count = 1;
                    
                    Serial.printf("[Netdata] Disk: %d/%d GB (%d%%)\n",
                                  data.volumes[0].used_gb, data.volumes[0].total_gb, data.volumes[0].used_pct);
                }
            }
            return true;
        }
    }
    http->end();
    return false;
}

bool NetdataClient::isConnected() { return data.is_online; }
const NasData& NetdataClient::getData() { return data; }
const char* NetdataClient::getTypeName() { return "Netdata"; }
const char* NetdataClient::getConnIcon() { return "wifi"; }
