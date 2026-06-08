#include "truenas_client.h"
#include "config.h"
#include <ArduinoJson.h>
#include <WiFi.h>

TrueNASClient::TrueNASClient() {
    memset(&data, 0, sizeof(data));
    http = nullptr;
    api_key[0] = '\0';
    state = TRUENAS_IDLE;
    consecutive_failures = 0;
}

TrueNASClient::~TrueNASClient() {
    if (http) { http->end(); delete http; }
}

bool TrueNASClient::init(Preferences& prefs) {
    prefs.getString(NVS_NAS_IP, nas_ip, sizeof(nas_ip));
    nas_port = prefs.getUShort(NVS_NAS_PORT, 0);
    if (nas_port == 0) nas_port = 80; // Default HTTP port
    prefs.getString(NVS_NAS_PASS, api_key, sizeof(api_key)); // API key stored in password field
    http = new HTTPClient();
    http->setTimeout(5000);
    Serial.printf("[TrueNAS] Init: %s:%d\n", nas_ip, nas_port);
    return strlen(nas_ip) > 0 && strlen(api_key) > 0;
}

bool TrueNASClient::connect() {
    Serial.println("[TrueNAS] Connecting...");
    // 尝试获取系统信息来验证连接
    if (fetchSystem()) {
        Serial.println("[TrueNAS] Connected successfully");
        data.is_online = true;
        state = TRUENAS_FETCH_POOLS;
        return true;
    } else {
        Serial.println("[TrueNAS] Connection failed, will retry in poll()");
        data.is_online = false;
        consecutive_failures = 1;
        state = TRUENAS_IDLE;
        return false;
    }
}

bool TrueNASClient::poll() {
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
        if (fetchSystem()) {
            Serial.println("[TrueNAS] Reconnected");
            data.is_online = true;
            consecutive_failures = 0;
            state = TRUENAS_FETCH_POOLS;
        } else {
            consecutive_failures++;
            Serial.printf("[TrueNAS] Reconnect failed (failures: %d, next retry in %ds)\n", 
                         consecutive_failures, poll_interval/1000);
            last_poll_ms = now;
            return false;
        }
    }
    
    bool success = false;
    
    switch (state) {
        case TRUENAS_IDLE:
            state = TRUENAS_FETCH_SYSTEM;
            // Fall through
            
        case TRUENAS_FETCH_SYSTEM:
            success = fetchSystem();
            if (success) {
                state = TRUENAS_FETCH_POOLS;
            } else {
                data.is_online = false;
                consecutive_failures++;
            }
            break;
            
        case TRUENAS_FETCH_POOLS:
            success = fetchPools();
            if (success) {
                state = TRUENAS_FETCH_DISKS;
            } else {
                consecutive_failures++;
            }
            break;
            
        case TRUENAS_FETCH_DISKS:
            success = fetchDisks();
            if (success) {
                state = TRUENAS_FETCH_INTERFACES;
            } else {
                consecutive_failures++;
            }
            break;
            
        case TRUENAS_FETCH_INTERFACES:
            success = fetchInterfaces();
            if (success) {
                // 成功后重置失败计数
                if (consecutive_failures > 0) {
                    Serial.printf("[TrueNAS] Connection recovered after %d failures\n", consecutive_failures);
                    consecutive_failures = 0;
                }
                data.is_online = true;
                data.last_update_ms = now;
                data.has_update = true;
                state = TRUENAS_DONE;
                Serial.println("[TrueNAS] Poll complete");
            } else {
                consecutive_failures++;
            }
            break;
            
        case TRUENAS_DONE:
            state = TRUENAS_FETCH_SYSTEM;
            last_poll_ms = now;
            return false;
    }
    
    last_poll_ms = now;
    return data.has_update;
}

bool TrueNASClient::fetchSystem() {
    if (!WiFi.isConnected()) {
        data.is_online = false;
        return false;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d/api/v2.0/system/info", nas_ip, nas_port);
    http->begin(url);
    http->addHeader("Content-Type", "application/json");
    
    char auth_header[128];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", api_key);
    http->addHeader("Authorization", auth_header);
    
    int code = http->GET();
    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[1024];  // 大幅减少
        WiFiClient* stream = http->getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, resp);
        http->end();
        if (!err) {
            // hostname
            const char* hostname = doc["hostname"].as<const char*>();
            if (hostname) strlcpy(data.system.hostname, hostname, sizeof(data.system.hostname));
            
            // uptime (seconds)
            data.system.uptime_s = doc["uptime_seconds"].as<uint32_t>();
            
            // CPU
            data.system.cpu_pct = doc["loadavg"].as<float>(); // Approximation
            
            // Memory (bytes)
            uint64_t mem_total = doc["physmem"].as<uint64_t>();
            uint64_t mem_used = doc["buffers"].as<uint64_t>() + doc["cached"].as<uint64_t>();
            
            data.system.ram_total_mb = (uint32_t)(mem_total / (1024 * 1024));
            data.system.ram_used_mb = (uint32_t)(mem_used / (1024 * 1024));
            data.system.ram_free_mb = data.system.ram_total_mb - data.system.ram_used_mb;
            
            if (data.system.ram_total_mb > 0) {
                data.system.ram_pct = (float)data.system.ram_used_mb / data.system.ram_total_mb * 100.0f;
            }
            
            Serial.printf("[TrueNAS] System: %s, Up: %lu, RAM: %d/%d MB\n",
                          data.system.hostname, data.system.uptime_s,
                          data.system.ram_used_mb, data.system.ram_total_mb);
            return true;
        }
    }
    http->end();
    return false;
}

bool TrueNASClient::fetchPools() {
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d/api/v2.0/pool", nas_ip, nas_port);
    http->begin(url);
    http->addHeader("Content-Type", "application/json");
    
    char auth_header[128];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", api_key);
    http->addHeader("Authorization", auth_header);
    
    int code = http->GET();
    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[1024];  // 大幅减少
        WiFiClient* stream = http->getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, resp);
        http->end();
        if (!err) {
            JsonArray pools = doc.as<JsonArray>();
            data.volume_count = 0;
            for (JsonObject pool : pools) {
                if (data.volume_count >= MAX_VOLUMES) break;
                
                const char* name = pool["name"].as<const char*>();
                if (name) strlcpy(data.volumes[data.volume_count].name, name, sizeof(data.volumes[data.volume_count].name));
                
                // Parse size from stats or properties
                JsonObject props = pool["properties"].as<JsonObject>();
                if (!props.isNull()) {
                    uint64_t size = props["allocated"]["value"].as<uint64_t>() + 
                                    props["free"]["value"].as<uint64_t>();
                    uint64_t used = props["allocated"]["value"].as<uint64_t>();
                    
                    data.volumes[data.volume_count].total_gb = (uint32_t)(size / (1024ULL * 1024ULL * 1024ULL));
                    data.volumes[data.volume_count].used_gb = (uint32_t)(used / (1024ULL * 1024ULL * 1024ULL));
                    
                    if (data.volumes[data.volume_count].total_gb > 0) {
                        data.volumes[data.volume_count].used_pct = 
                            (data.volumes[data.volume_count].used_gb * 100) / data.volumes[data.volume_count].total_gb;
                    }
                }
                
                const char* status = pool["status"].as<const char*>();
                if (status) strlcpy(data.volumes[data.volume_count].status, status, sizeof(data.volumes[data.volume_count].status));
                
                strlcpy(data.volumes[data.volume_count].raid, "zfs", sizeof(data.volumes[data.volume_count].raid));
                data.volume_count++;
            }
            
            Serial.printf("[TrueNAS] Pools: %d\n", data.volume_count);
            return true;
        }
    }
    http->end();
    return false;
}

bool TrueNASClient::fetchDisks() {
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d/api/v2.0/disk", nas_ip, nas_port);
    http->begin(url);
    http->addHeader("Content-Type", "application/json");
    
    char auth_header[128];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", api_key);
    http->addHeader("Authorization", auth_header);
    
    int code = http->GET();
    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[2048];  // 大幅减少
        WiFiClient* stream = http->getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, resp);
        http->end();
        if (!err) {
            JsonArray disks = doc.as<JsonArray>();
            data.disk_count = 0;
            for (JsonObject disk : disks) {
                if (data.disk_count >= MAX_DISKS) break;
                
                const char* name = disk["name"].as<const char*>();
                if (name) strlcpy(data.disks[data.disk_count].name, name, sizeof(data.disks[data.disk_count].name));
                
                const char* model = disk["model"].as<const char*>();
                if (model) strlcpy(data.disks[data.disk_count].model_name, model, sizeof(data.disks[data.disk_count].model_name));
                
                // Size in bytes
                uint64_t size = disk["size"].as<uint64_t>();
                data.disks[data.disk_count].size_gb = (uint32_t)(size / (1024ULL * 1024ULL * 1024ULL));
                
                // Temperature (if available)
                int temp = disk["temperature"].as<int>();
                if (temp > 0) data.disks[data.disk_count].temp = (int16_t)temp;
                
                // Health (simplified - assume OK if disk is present)
                data.disks[data.disk_count].health = HEALTH_OK;
                
                data.disk_count++;
            }
            
            Serial.printf("[TrueNAS] Disks: %d\n", data.disk_count);
            return true;
        }
    }
    http->end();
    return false;
}

bool TrueNASClient::fetchInterfaces() {
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d/api/v2.0/interface", nas_ip, nas_port);
    http->begin(url);
    http->addHeader("Content-Type", "application/json");
    
    char auth_header[128];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", api_key);
    http->addHeader("Authorization", auth_header);
    
    int code = http->GET();
    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[1024];  // 大幅减少
        WiFiClient* stream = http->getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, resp);
        http->end();
        if (!err) {
            // Find the primary interface with IP address
            JsonObject interfaces = doc.as<JsonObject>();
            for (JsonPair kv : interfaces) {
                JsonObject iface = kv.value().as<JsonObject>();
                JsonArray addresses = iface["aliases"].as<JsonArray>();
                
                for (JsonObject addr : addresses) {
                    const char* type = addr["type"].as<const char*>();
                    if (type && strcmp(type, "INET") == 0) { // IPv4
                        const char* ip = addr["address"].as<const char*>();
                        if (ip) {
                            strlcpy(data.network.ip, ip, sizeof(data.network.ip));
                            strlcpy(data.network.interface, kv.key().c_str(), sizeof(data.network.interface));
                            Serial.printf("[TrueNAS] Interface: %s (%s)\n", data.network.interface, data.network.ip);
                            return true;
                        }
                    }
                }
            }
            return true;
        }
    }
    http->end();
    return false;
}

bool TrueNASClient::isConnected() { return data.is_online; }
const NasData& TrueNASClient::getData() { return data; }
const char* TrueNASClient::getTypeName() { return "TrueNAS"; }
const char* TrueNASClient::getConnIcon() { return "wifi"; }
