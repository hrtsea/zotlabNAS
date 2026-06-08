// ZotLab NAS Monitor - Serial Client Implementation
#include "serial_client.h"
#include "config.h"
#include "nas_config.h"
#include "user_config.h"

SerialClient::SerialClient()
    : serial(nullptr), baud_rate(DEFAULT_SERIAL_BAUD),
      state(SERIAL_IDLE), last_frame_ms(0), rx_pos(0), connected(false) {
    memset(rx_buffer, 0, sizeof(rx_buffer));
    clearData();
}

SerialClient::~SerialClient() {
    if (serial) {
        serial->end();
        delete serial;
    }
}

bool SerialClient::init(Preferences& prefs) {
    baud_rate = prefs.getUInt(NVS_SERIAL_BAUD, DEFAULT_SERIAL_BAUD);
    serial = new HardwareSerial(2);
    clearData();
    Serial.printf("[SerialClient] Init: baud=%lu\n", baud_rate);
    return true;
}

bool SerialClient::connect() {
    if (!serial) return false;
    serial->begin(baud_rate, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN);
    connected = true;
    state = SERIAL_WAITING_FRAME;
    Serial.printf("[SerialClient] Connected: UART2 RX=%d TX=%d @ %lu\n",
                  UART2_RX_PIN, UART2_TX_PIN, baud_rate);
    return true;
}

bool SerialClient::poll() {
    if (!connected) return false;

    // Check for timeout (no frame received)
    uint32_t now = millis();
    if (last_frame_ms > 0 && (now - last_frame_ms) > SERIAL_TIMEOUT_MS) {
        if (data.is_online) {
            data.is_online = false;
            Serial.println(F("[SerialClient] Timeout - no data received"));
        }
    }

    // Read available data
    if (serial->available()) {
        while (serial->available() > 0 && rx_pos < SERIAL_BUF_SIZE - 1) {
            uint8_t c = serial->read();

            if (c == SERIAL_STX) {
                // Start of frame - reset buffer
                rx_pos = 0;
                state = SERIAL_RECEIVING;
            } else if (c == SERIAL_ETX && state == SERIAL_RECEIVING) {
                // End of frame
                rx_buffer[rx_pos] = '\0';
                state = SERIAL_PARSING;
                if (parseJSON(rx_buffer)) {
                    data.is_online = true;
                    data.last_update_ms = now;
                    data.has_update = true;
                    last_frame_ms = now;
                }
                state = SERIAL_WAITING_FRAME;
                rx_pos = 0;
                break; // Frame complete
            } else if (state == SERIAL_RECEIVING) {
                rx_buffer[rx_pos++] = (char)c;
            }
        }
    }

    return data.has_update;
}

bool SerialClient::parseJSON(const char* json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[SerialClient] JSON parse error: %s\n", err.c_str());
        return false;
    }

    // System info (same schema as HTTP API)
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

        if (sys["cpu_cores"].is<JsonArray>()) {
            JsonArray cores = sys["cpu_cores"];
            data.system.cpu_core_count = min((size_t)cores.size(), (size_t)8);
            for (uint8_t i = 0; i < data.system.cpu_core_count; i++) {
                data.system.cpu_cores[i] = cores[i] | 0.0f;
            }
        }
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

    return true;
}

bool SerialClient::isConnected() {
    return connected && data.is_online;
}

const NasData& SerialClient::getData() {
    return data;
}

const char* SerialClient::getTypeName() {
    return "Linux (Serial)";
}

const char* SerialClient::getConnIcon() {
    return "usb";
}

void SerialClient::clearData() {
    memset(&data, 0, sizeof(data));
    data.system.temp_cpu = -1;
    data.system.temp_sys = -1;
    for (int i = 0; i < MAX_DISKS; i++) data.disks[i].temp = -1;
}
