#include "synology_client.h"
#include "config.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

SynologyClient::SynologyClient() {
    memset(&data, 0, sizeof(data));
    sid[0] = '\0';
    consecutive_failures = 0;
}

SynologyClient::~SynologyClient() {}

bool SynologyClient::init(Preferences& prefs) {
    prefs.getString(NVS_NAS_IP, nas_ip, sizeof(nas_ip));
    nas_port = prefs.getUShort(NVS_NAS_PORT, 5000);
    prefs.getString(NVS_NAS_USER, username, sizeof(username));
    prefs.getString(NVS_NAS_PASS, password, sizeof(password));
    use_https = prefs.getBool(NVS_NAS_HTTPS, false);
    Serial.printf("[Synology] Init: %s:%d\n", nas_ip, nas_port);
    return strlen(nas_ip) > 0;
}

bool SynologyClient::connect() {
    // 尝试登录,如果失败也不阻塞,由poll()处理重试
    Serial.println("[Synology] Connecting...");
    bool success = login();
    if (success) {
        Serial.println("[Synology] Connected successfully");
        data.is_online = true;
    } else {
        Serial.println("[Synology] Connection failed, will retry in poll()");
        data.is_online = false;
        consecutive_failures = 1;  // 设置初始失败计数,触发退避
    }
    return success;
}

bool SynologyClient::poll() {
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
            Serial.println("[Synology] Reconnected");
            data.is_online = true;
            consecutive_failures = 0;
        } else {
            consecutive_failures++;
            Serial.printf("[Synology] Reconnect failed (failures: %d, next retry in %ds)\n", 
                         consecutive_failures, poll_interval/1000);
            last_poll_ms = now;
            return false;
        }
    }

    // 已连接,获取数据
    if (!fetchSystem()) { 
        data.is_online = false;
        consecutive_failures++;
        Serial.printf("[Synology] Poll failed (consecutive: %d, next retry in %ds)\n", 
                     consecutive_failures, poll_interval/1000);
        last_poll_ms = now;
        return false; 
    }
    
    // 成功后重置失败计数
    if (consecutive_failures > 0) {
        Serial.printf("[Synology] Connection recovered after %d failures\n", consecutive_failures);
        consecutive_failures = 0;
    }
    
    fetchStorage();
    data.is_online = true;
    data.last_update_ms = now;
    data.has_update = true;
    last_poll_ms = now;
    return true;
}

bool SynologyClient::login() {
    if (!WiFi.isConnected()) {
        Serial.println("[Synology] WiFi not connected, skip login");
        return false;
    }
    
    char url[384];
    if (use_https) {
        snprintf(url, sizeof(url),
            "https://%s:%d/webapi/auth.cgi?api=SYNO.API.Auth&version=7&method=login&account=%s&passwd=%s&format=sid",
            nas_ip, nas_port, username, password);
    } else {
        snprintf(url, sizeof(url),
            "http://%s:%d/webapi/auth.cgi?api=SYNO.API.Auth&version=7&method=login&account=%s&passwd=%s&format=sid",
            nas_ip, nas_port, username, password);
    }

    HTTPClient http;
    http.begin(url);
    http.setTimeout(3000);  // 3秒超时,避免阻塞UI
    int code = http.GET();

    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[512];  // 大幅减少
        WiFiClient* stream = http.getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, resp);
        if (!err && doc["success"].as<bool>()) {
            const char* s = doc["data"]["sid"].as<const char*>();
            if (s && strlen(s) > 0) {
                strlcpy(sid, s, sizeof(sid));
                http.end();
                Serial.println("[Synology] Logged in");
                return true;
            }
        }
    }
    http.end();
    Serial.printf("[Synology] Login failed\n");
    return false;
}

bool SynologyClient::fetchSystem() {
    if (!WiFi.isConnected()) {
        data.is_online = false;
        return false;
    }
    
    char url[384];
    snprintf(url, sizeof(url),
        "http://%s:%d/webapi/entry.cgi?api=SYNO.Core.System.Utilization&version=1&method=get&_sid=%s",
        nas_ip, nas_port, sid);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(3000);  // 3秒超时,避免阻塞UI
    int code = http.GET();

    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[1024];  // 大幅减少
        WiFiClient* stream = http.getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, resp);
        http.end();

        if (!err && doc["success"].as<bool>()) {
            auto d = doc["data"].as<JsonObject>();
            auto cpu = d["cpu"].as<JsonObject>();
            auto mem = d["memory"].as<JsonObject>();
            data.system.cpu_pct = cpu["user_load"].as<float>() + cpu["system_load"].as<float>();
            data.system.ram_pct = mem["real_usage"].as<float>();
            data.system.ram_total_mb = mem["memory_size"].as<uint32_t>();
            data.system.ram_used_mb = (uint32_t)(data.system.ram_total_mb * data.system.ram_pct / 100.0f);
            return true;
        }
    }
    http.end();
    return false;
}

bool SynologyClient::fetchStorage() {
    if (!WiFi.isConnected()) {
        data.is_online = false;
        return false;
    }
    
    char url[384];
    snprintf(url, sizeof(url),
        "http://%s:%d/webapi/entry.cgi?api=SYNO.Storage.CGI.Storage&version=1&method=get&_sid=%s",
        nas_ip, nas_port, sid);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(3000);  // 3秒超时,避免阻塞UI
    int code = http.GET();

    if (code == 200) {
        // 使用固定缓冲区接收响应
        static char resp[1024];  // 大幅减少
        WiFiClient* stream = http.getStreamPtr();
        size_t len = stream->readBytes(resp, sizeof(resp) - 1);
        resp[len] = '\0';
        
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, resp);
        http.end();

        if (!err && doc["success"].as<bool>()) {
            auto disks_arr = doc["data"]["disks"].as<JsonArray>();
            if (!disks_arr.isNull()) {
                data.disk_count = min((size_t)disks_arr.size(), (size_t)MAX_DISKS);
                for (size_t i = 0; i < data.disk_count; i++) {
                    auto disk = disks_arr[i].as<JsonObject>();
                    strlcpy(data.disks[i].name, disk["name"].as<const char*>() ?: "", sizeof(data.disks[i].name));
                    data.disks[i].temp = disk["temp"].as<int>();
                    const char* h = disk["smart_status"].as<const char*>() ?: "normal";
                    data.disks[i].health = (strcmp(h, "normal") == 0) ? HEALTH_OK : HEALTH_WARNING;
                }
            }
        }
    }
    http.end();
    return true;
}

bool SynologyClient::isConnected() { return data.is_online; }
const NasData& SynologyClient::getData() { return data; }
const char* SynologyClient::getTypeName() { return "Synology DSM"; }
const char* SynologyClient::getConnIcon() { return "wifi"; }
