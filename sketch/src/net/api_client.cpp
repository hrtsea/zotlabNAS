// ZotLab NAS Monitor - HTTP API Client Implementation
#include <WiFi.h>
#include "api_client.h"
#include "config.h"
#include "nas_config.h"

ApiClient::ApiClient()
    : nas_port(0), state(API_IDLE), last_poll_ms(0),
      poll_interval_ms(0), http(nullptr), consecutive_failures(0), current_type(NET_LINUX_HTTP) {
    memset(&data, 0, sizeof(data));
    clearData();
}

ApiClient::~ApiClient() {
    if (http) {
        http->end();
        delete http;
    }
}

bool ApiClient::init(Preferences& prefs) {
    // Load NAS IP and port from config
    prefs.getString(NVS_NAS_IP, nas_ip, sizeof(nas_ip));
    nas_port = prefs.getUShort(NVS_NAS_PORT, 0);
    if (nas_port == 0) nas_port = DEFAULT_HTTP_PORT;

    // Set poll interval from config
    poll_interval_ms = prefs.getUChar(NVS_POLL_SEC, DEFAULT_POLL_SEC) * 1000UL;

    http = new HTTPClient();
    http->setTimeout(5000);

    Serial.printf("[ApiClient] Init: %s:%d\n", nas_ip, nas_port);
    return strlen(nas_ip) > 0;
}

bool ApiClient::connect() {
    if (strlen(nas_ip) == 0) return false;
    
    Serial.println("[Api] Connecting...");
    // 尝试获取数据来验证连接
    if (fetchAll()) {
        Serial.printf("[Api] Connected successfully: %s:%d\n", nas_ip, nas_port);
        data.is_online = true;
        state = API_IDLE;
        return true;
    } else {
        Serial.println("[Api] Connection failed, will retry in poll()");
        data.is_online = false;
        consecutive_failures = 1;
        state = API_IDLE;
        return false;
    }
}

bool ApiClient::poll() {
    uint32_t now = millis();

    // 退避机制: 连续失败后延长轮询间隔
    uint32_t current_interval = poll_interval_ms;
    if (consecutive_failures > 0) {
        // 限制指数最大阶数,避免位移溢出(最多2^3=8倍)
        uint8_t capped = consecutive_failures;
        if (capped > 3) capped = 3;
        
        uint32_t backoff = poll_interval_ms * (1u << capped);
        if (backoff > 60000) backoff = 60000;  // 钳制到60秒上限
        current_interval = backoff;
    }

    // Check if it's time to poll
    if (now - last_poll_ms < current_interval && last_poll_ms > 0) {
        return false;
    }
    
    // 如果未连接,尝试重新连接
    if (!data.is_online) {
        if (fetchAll()) {
            Serial.println("[Api] Reconnected");
            data.is_online = true;
            consecutive_failures = 0;
        } else {
            consecutive_failures++;
            Serial.printf("[Api] Reconnect failed (failures: %d, next retry in %ums)\n", 
                         consecutive_failures, current_interval);
            last_poll_ms = now;
            return false;
        }
    }

    switch (state) {
        case API_IDLE:
            if (fetchAll()) {
                // 成功后重置失败计数
                if (consecutive_failures > 0) {
                    Serial.printf("[ApiClient] Connection recovered after %d failures\n", consecutive_failures);
                    consecutive_failures = 0;
                }
                state = API_REQUESTING;
            } else {
                consecutive_failures++;
                Serial.printf("[ApiClient] Poll failed (consecutive: %d)\n", consecutive_failures);
            }
            break;

        case API_REQUESTING:
            // fetchAll is synchronous for now (HTTPClient is blocking)
            // In a future optimization, this could be made non-blocking
            state = API_DONE;
            break;

        case API_DONE:
            last_poll_ms = now;
            state = API_IDLE;
            return true; // data updated
        default:
            state = API_IDLE;
            break;
    }

    return false;
}

bool ApiClient::fetchAll() {
    if (!WiFi.isConnected()) {
        data.is_online = false;
        return false;
    }

    char url[128];
    snprintf(url, sizeof(url), "http://%s:%d/api/all", nas_ip, nas_port);

    http->begin(url);
    int httpCode = http->GET();

    if (httpCode == HTTP_CODE_OK) {
        // 使用固定缓冲区接收响应，避免String堆碎片
        static char payload[1024];  // 大幅减少以节省DRAM
        WiFiClient* stream = http->getStreamPtr();
        size_t len = stream->readBytes(payload, sizeof(payload) - 1);
        payload[len] = '\0';
        
        bool parsed = parseJSON(payload);
        if (parsed) {
            data.is_online = true;
            data.last_update_ms = millis();
            data.has_update = true;
        }
        http->end();
        return parsed;
    }

    Serial.printf("[ApiClient] HTTP error: %d\n", httpCode);
    http->end();
    data.is_online = false;
    return false;
}

bool ApiClient::parseJSON(const char* json) {
    // 使用JsonDocument（ArduinoJson v7推荐），栈上分配
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[ApiClient] JSON parse error: %s\n", err.c_str());
        return false;
    }

    // System info
    if (doc["system"].is<JsonObject>()) {
        JsonObject sys = doc["system"];
        strlcpy(data.system.hostname, sys["hostname"] | "unknown", sizeof(data.system.hostname));
        strlcpy(data.system.model, sys["model"] | "Custom", sizeof(data.system.model));
        data.system.uptime_s = sys["uptime_s"] | 0;
        data.system.cpu_pct = sys["cpu_pct"] | 0.0f;
        data.system.ram_pct = sys["ram_pct"] | 0.0f;
        data.system.ram_total_mb = sys["ram_total_mb"] | 0;
        data.system.ram_used_mb = sys["ram_used_mb"] | 0;
        data.system.ram_free_mb = sys["ram_free_mb"] | 0;
        data.system.ram_cached_mb = sys["ram_cached_mb"] | 0;
        data.system.swap_total_mb = sys["swap_total_mb"] | 0;
        data.system.swap_used_mb = sys["swap_used_mb"] | 0;
        data.system.temp_cpu = sys["temp_cpu"] | -1;
        data.system.temp_sys = sys["temp_sys"] | -1;

        // CPU cores
        if (sys["cpu_cores"].is<JsonArray>()) {
            JsonArray cores = sys["cpu_cores"];
            data.system.cpu_core_count = min((size_t)cores.size(), (size_t)8);
            for (uint8_t i = 0; i < data.system.cpu_core_count; i++) {
                data.system.cpu_cores[i] = cores[i] | 0.0f;
            }
        }

        // Load average
        if (sys["load_avg"].is<JsonArray>()) {
            JsonArray load = sys["load_avg"];
            for (uint8_t i = 0; i < 3 && i < load.size(); i++) {
                data.system.load_avg[i] = load[i] | 0.0f;
            }
        }
    }

    // Disks
    data.disk_count = 0;
    if (doc["disks"].is<JsonArray>()) {
        JsonArray disks = doc["disks"];
        for (size_t i = 0; i < disks.size() && i < MAX_DISKS; i++) {
            JsonObject d = disks[i];
            strlcpy(data.disks[i].name, d["name"] | "", sizeof(data.disks[i].name));
            strlcpy(data.disks[i].device, d["device"] | "", sizeof(data.disks[i].device));
            strlcpy(data.disks[i].model_name, d["model"] | "", sizeof(data.disks[i].model_name));
            strlcpy(data.disks[i].mount, d["mount"] | "", sizeof(data.disks[i].mount));
            strlcpy(data.disks[i].disk_type, d["disk_type"] | "HDD", sizeof(data.disks[i].disk_type));
            data.disks[i].temp = d["temp"] | -1;
            data.disks[i].size_gb = d["size_gb"] | 0;
            data.disks[i].used_gb = d["used_gb"] | 0;
            data.disks[i].used_pct = d["used_pct"] | 0;

            const char* health = d["health"] | "OK";
            if (strcmp(health, "OK") == 0) data.disks[i].health = HEALTH_OK;
            else if (strcmp(health, "WARNING") == 0) data.disks[i].health = HEALTH_WARNING;
            else data.disks[i].health = HEALTH_CRITICAL;

            data.disk_count++;
        }
    }

    // Volumes
    data.volume_count = 0;
    if (doc["volumes"].is<JsonArray>()) {
        JsonArray vols = doc["volumes"];
        for (size_t i = 0; i < vols.size() && i < MAX_VOLUMES; i++) {
            JsonObject v = vols[i];
            strlcpy(data.volumes[i].name, v["name"] | "", sizeof(data.volumes[i].name));
            data.volumes[i].total_gb = v["total_gb"] | 0;
            data.volumes[i].used_gb = v["used_gb"] | 0;
            data.volumes[i].used_pct = v["used_pct"] | 0;
            strlcpy(data.volumes[i].raid, v["raid"] | "none", sizeof(data.volumes[i].raid));
            strlcpy(data.volumes[i].status, v["status"] | "normal", sizeof(data.volumes[i].status));
            data.volume_count++;
        }
    }

    // Services
    data.service_count = 0;
    if (doc["services"].is<JsonArray>()) {
        JsonArray svcs = doc["services"];
        for (size_t i = 0; i < svcs.size() && i < MAX_SERVICES; i++) {
            JsonObject s = svcs[i];
            strlcpy(data.services[i].name, s["name"] | "", sizeof(data.services[i].name));
            data.services[i].running = s["running"] | false;
            data.services[i].is_docker = s["is_docker"] | false;
            data.service_count++;
        }
    }

    // Network
    if (doc["network"].is<JsonObject>()) {
        JsonObject net = doc["network"];
        strlcpy(data.network.interface, net["interface"] | "eth0", sizeof(data.network.interface));
        strlcpy(data.network.ip, net["ip"] | "0.0.0.0", sizeof(data.network.ip));
        data.network.rx_bps = net["rx_bps"] | 0;
        data.network.tx_bps = net["tx_bps"] | 0;
    }

    state = API_DONE;
    return true;
}

bool ApiClient::isConnected() {
    return data.is_online && strlen(nas_ip) > 0;
}

const NasData& ApiClient::getData() {
    return data;
}

const char* ApiClient::getTypeName() {
    return "Linux (HTTP)";
}

const char* ApiClient::getConnIcon() {
    return "wifi";
}

void ApiClient::clearData() {
    memset(&data, 0, sizeof(data));
    data.system.temp_cpu = -1;
    data.system.temp_sys = -1;
    for (int i = 0; i < MAX_DISKS; i++) data.disks[i].temp = -1;
}

NasTypeConfig ApiClient::getConfig() {
    return NasTypeConfig::getDefaults(current_type);
}

void ApiClient::setType(NasType type) {
    current_type = type;
}
