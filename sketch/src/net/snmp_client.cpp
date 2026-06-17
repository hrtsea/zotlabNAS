#include "snmp_client.h"
#include "data/config.h"
#include "data/nas_config.h"
#include <WiFi.h>

// OID definitions (BER encoded)
static const uint8_t OID_SYSNAME[] = {0x2B, 0x06, 0x01, 0x02, 0x01, 0x01, 0x05, 0x00};
static const uint8_t OID_SYSUPTIME[] = {0x2B, 0x06, 0x01, 0x02, 0x01, 0x01, 0x03, 0x00};
static const uint8_t OID_HRPROCLOAD[] = {0x2B, 0x06, 0x01, 0x02, 0x01, 0x25, 0x03, 0x03, 0x01, 0x02, 0x01};
static const uint8_t OID_IFINOCTETS[] = {0x2B, 0x06, 0x01, 0x02, 0x01, 0x02, 0x02, 0x01, 0x0A, 0x02};
static const uint8_t OID_IFOUTOCTETS[] = {0x2B, 0x06, 0x01, 0x02, 0x01, 0x02, 0x02, 0x01, 0x10, 0x02};

SnmpClient::SnmpClient() : udp(nullptr), nas_port(161), state(SNMP_IDLE), last_query(SNMP_IDLE), request_id(0), rx_len(0) {
    memset(&data, 0, sizeof(data));
    memset(nas_ip, 0, sizeof(nas_ip));
    memset(community, 0, sizeof(community));
    memset(rx_buf, 0, sizeof(rx_buf));
    strlcpy(community, "public", sizeof(community));
}

SnmpClient::~SnmpClient() {
    if (udp) delete udp;
}

bool SnmpClient::init(Preferences& prefs) {
    prefs.getString(NVS_NAS_IP, nas_ip, sizeof(nas_ip));
    nas_port = (uint16_t)prefs.getUShort(NVS_NAS_PORT, 161);
    prefs.getString(NVS_SNMP_COMM, community, sizeof(community));
    if (strlen(community) == 0) strlcpy(community, "public", sizeof(community));
    
    // 检查内存是否足够
    Serial.printf("[SNMP] Heap before WiFiUDP: %u bytes\n", ESP.getFreeHeap());
    
    udp = new (std::nothrow) WiFiUDP();
    if (!udp) {
        Serial.println("[SNMP] ERROR: Failed to allocate WiFiUDP");
        return false;
    }
    
    request_id = random(1, 65535);
    Serial.printf("[SNMP] Init: %s:%d (community=%s)\n", nas_ip, nas_port, community);
    return strlen(nas_ip) > 0;
}

bool SnmpClient::connect() {
    if (!udp) {
        Serial.println("[SNMP] ERROR: UDP not initialized");
        return false;
    }
    
    if (udp->begin(0)) {
        Serial.println("[SNMP] UDP socket opened");
        data.is_online = true;
        state = SNMP_IDLE;
        return true;
    }
    return false;
}

// BER length encoding
static size_t encodeLength(uint16_t len, uint8_t* buf) {
    if (len < 128) {
        buf[0] = (uint8_t)len;
        return 1;
    }
    buf[0] = 0x82;
    buf[1] = (len >> 8) & 0xFF;
    buf[2] = len & 0xFF;
    return 3;
}

// Build SNMP GET request
bool SnmpClient::sendGet(const uint8_t* oid, size_t oid_len) {
    uint8_t tx_buf[256];
    size_t pos = 0;
    
    request_id++;
    
    // Build VarBind: SEQUENCE { OID, Null }
    uint8_t varbind[64];
    size_t vb_pos = 0;
    varbind[vb_pos++] = 0x30; // SEQUENCE
    vb_pos++; // length placeholder
    
    // OID
    varbind[vb_pos++] = 0x06; // OID tag
    varbind[vb_pos++] = (uint8_t)oid_len;
    memcpy(varbind + vb_pos, oid, oid_len);
    vb_pos += oid_len;
    
    // Null value
    varbind[vb_pos++] = 0x05;
    varbind[vb_pos++] = 0x00;
    
    varbind[1] = (uint8_t)(vb_pos - 2);
    
    // VarBind list
    uint8_t vblist[68];
    vblist[0] = 0x30;
    vblist[1] = vb_pos;
    memcpy(vblist + 2, varbind, vb_pos);
    size_t vblist_len = 2 + vb_pos;
    
    // PDU: GetRequest-PDU
    uint8_t pdu[128];
    size_t pdu_pos = 0;
    pdu[pdu_pos++] = 0xA0;
    pdu_pos++;
    
    pdu[pdu_pos++] = 0x02; // request-id
    pdu[pdu_pos++] = 0x02;
    pdu[pdu_pos++] = (request_id >> 8) & 0xFF;
    pdu[pdu_pos++] = request_id & 0xFF;
    
    pdu[pdu_pos++] = 0x02; // error-status
    pdu[pdu_pos++] = 0x01;
    pdu[pdu_pos++] = 0x00;
    
    pdu[pdu_pos++] = 0x02; // error-index
    pdu[pdu_pos++] = 0x01;
    pdu[pdu_pos++] = 0x00;
    
    memcpy(pdu + pdu_pos, vblist, vblist_len);
    pdu_pos += vblist_len;
    pdu[1] = (uint8_t)(pdu_pos - 2);
    
    // Full message
    tx_buf[pos++] = 0x30;
    pos++;
    
    // version (v2c = 1)
    tx_buf[pos++] = 0x02;
    tx_buf[pos++] = 0x01;
    tx_buf[pos++] = 0x01;
    
    // community
    size_t comm_len = strlen(community);
    tx_buf[pos++] = 0x04;
    tx_buf[pos++] = (uint8_t)comm_len;
    memcpy(tx_buf + pos, community, comm_len);
    pos += comm_len;
    
    // PDU
    memcpy(tx_buf + pos, pdu, pdu_pos);
    pos += pdu_pos;
    tx_buf[1] = (uint8_t)(pos - 2);
    
    udp->beginPacket(nas_ip, nas_port);
    udp->write(tx_buf, pos);
    return udp->endPacket();
}

bool SnmpClient::waitForResponse(uint32_t timeout_ms) {
    uint32_t start = millis();
    while (millis() - start < timeout_ms) {
        int pkt_len = udp->parsePacket();
        if (pkt_len > 0) {
            rx_len = udp->read(rx_buf, sizeof(rx_buf));
            return rx_len > 0;
        }
        delay(10);
    }
    return false;
}

size_t SnmpClient::decodeLength(const uint8_t* buf, size_t* out_len) {
    if (buf[0] < 128) {
        *out_len = buf[0];
        return 1;
    }
    size_t num_bytes = buf[0] & 0x7F;
    size_t len = 0;
    for (size_t i = 0; i < num_bytes; i++) {
        len = (len << 8) | buf[1 + i];
    }
    *out_len = len;
    return 1 + num_bytes;
}

int SnmpClient::decodeInteger(const uint8_t* buf, size_t len) {
    int val = 0;
    for (size_t i = 0; i < len; i++) {
        val = (val << 8) | buf[i];
    }
    return val;
}

bool SnmpClient::decodeOctetString(const uint8_t* buf, size_t len, char* out, size_t out_size) {
    if (len >= out_size) len = out_size - 1;
    memcpy(out, buf, len);
    out[len] = '\0';
    return true;
}

uint32_t SnmpClient::decodeCounter(const uint8_t* buf, size_t len) {
    uint32_t val = 0;
    for (size_t i = 0; i < len; i++) {
        val = (val << 8) | buf[i];
    }
    return val;
}

bool SnmpClient::findValueInResponse(const uint8_t* target_oid, size_t oid_len,
                                     const uint8_t** value_out, size_t* value_len, uint8_t* tag_out) {
    size_t pos = 0;
    
    if (pos >= (size_t)rx_len || rx_buf[pos++] != 0x30) return false;
    size_t msg_len;
    pos += decodeLength(rx_buf + pos, &msg_len);
    
    if (pos >= (size_t)rx_len || rx_buf[pos++] != 0x02) return false;
    size_t ver_len;
    pos += decodeLength(rx_buf + pos, &ver_len);
    pos += ver_len;
    
    if (pos >= (size_t)rx_len || rx_buf[pos++] != 0x04) return false;
    size_t comm_len;
    pos += decodeLength(rx_buf + pos, &comm_len);
    pos += comm_len;
    
    if (pos >= (size_t)rx_len) return false;
    pos++;
    size_t pdu_len;
    pos += decodeLength(rx_buf + pos, &pdu_len);
    
    for (int i = 0; i < 3; i++) {
        if (pos >= (size_t)rx_len || rx_buf[pos++] != 0x02) return false;
        size_t int_len;
        pos += decodeLength(rx_buf + pos, &int_len);
        pos += int_len;
    }
    
    if (pos >= (size_t)rx_len || rx_buf[pos++] != 0x30) return false;
    size_t vbl_len;
    pos += decodeLength(rx_buf + pos, &vbl_len);
    
    if (pos >= (size_t)rx_len || rx_buf[pos++] != 0x30) return false;
    size_t vb_len;
    pos += decodeLength(rx_buf + pos, &vb_len);
    
    if (pos >= (size_t)rx_len || rx_buf[pos++] != 0x06) return false;
    size_t resp_oid_len;
    pos += decodeLength(rx_buf + pos, &resp_oid_len);
    
    bool oid_match = (resp_oid_len == oid_len);
    if (oid_match) {
        for (size_t i = 0; i < oid_len; i++) {
            if (rx_buf[pos + i] != target_oid[i]) {
                oid_match = false;
                break;
            }
        }
    }
    
    pos += resp_oid_len;
    
    if (pos >= (size_t)rx_len) return false;
    *tag_out = rx_buf[pos++];
    size_t val_len;
    pos += decodeLength(rx_buf + pos, &val_len);
    
    if (oid_match && pos + val_len <= (size_t)rx_len) {
        *value_out = rx_buf + pos;
        *value_len = val_len;
        return true;
    }
    
    return false;
}

bool SnmpClient::poll() {
    if (!WiFi.isConnected()) {
        data.is_online = false;
        return false;
    }
    
    uint32_t now = millis();
    if (now - last_poll_ms < g_config.poll_sec * 1000UL) return false;
    
    uint8_t tag;
    const uint8_t* val;
    size_t val_len;
    
    switch (state) {
        case SNMP_IDLE:
            state = SNMP_QUERY_SYSNAME;
            
        case SNMP_QUERY_SYSNAME:
            Serial.println("[SNMP] Querying sysName...");
            sendGet(OID_SYSNAME, sizeof(OID_SYSNAME));
            last_query = SNMP_QUERY_SYSNAME;
            state = SNMP_WAIT_RESPONSE;
            break;
            
        case SNMP_QUERY_UPTIME:
            Serial.println("[SNMP] Querying sysUpTime...");
            sendGet(OID_SYSUPTIME, sizeof(OID_SYSUPTIME));
            last_query = SNMP_QUERY_UPTIME;
            state = SNMP_WAIT_RESPONSE;
            break;
            
        case SNMP_QUERY_CPU:
            Serial.println("[SNMP] Querying hrProcessorLoad...");
            sendGet(OID_HRPROCLOAD, sizeof(OID_HRPROCLOAD));
            last_query = SNMP_QUERY_CPU;
            state = SNMP_WAIT_RESPONSE;
            break;
            
        case SNMP_QUERY_IF_IN:
            Serial.println("[SNMP] Querying ifInOctets...");
            sendGet(OID_IFINOCTETS, sizeof(OID_IFINOCTETS));
            last_query = SNMP_QUERY_IF_IN;
            state = SNMP_WAIT_RESPONSE;
            break;
            
        case SNMP_QUERY_IF_OUT:
            Serial.println("[SNMP] Querying ifOutOctets...");
            sendGet(OID_IFOUTOCTETS, sizeof(OID_IFOUTOCTETS));
            last_query = SNMP_QUERY_IF_OUT;
            state = SNMP_WAIT_RESPONSE;
            break;
            
        case SNMP_WAIT_RESPONSE:
            if (waitForResponse(500)) {
                if (findValueInResponse(OID_SYSNAME, sizeof(OID_SYSNAME), &val, &val_len, &tag)) {
                    if (tag == 0x04 && val_len > 0) {
                        decodeOctetString(val, val_len, data.system.hostname, sizeof(data.system.hostname));
                        Serial.printf("[SNMP] Hostname: %s\n", data.system.hostname);
                    }
                }
                
                if (findValueInResponse(OID_SYSUPTIME, sizeof(OID_SYSUPTIME), &val, &val_len, &tag)) {
                    if (tag == 0x02 || tag == 0x43) {
                        uint32_t uptime = decodeCounter(val, val_len);
                        data.system.uptime_s = uptime / 100;
                        Serial.printf("[SNMP] Uptime: %lu seconds\n", data.system.uptime_s);
                    }
                }
                
                if (findValueInResponse(OID_HRPROCLOAD, sizeof(OID_HRPROCLOAD), &val, &val_len, &tag)) {
                    if (tag == 0x02) {
                        data.system.cpu_pct = (float)decodeInteger(val, val_len);
                        Serial.printf("[SNMP] CPU Load: %.1f%%\n", data.system.cpu_pct);
                    }
                }
                
                if (findValueInResponse(OID_IFINOCTETS, sizeof(OID_IFINOCTETS), &val, &val_len, &tag)) {
                    if (tag == 0x01 || tag == 0x41) {
                        uint32_t rx_bytes = decodeCounter(val, val_len);
                        if (last_net_time > 0) {
                            uint32_t dt = (now - last_net_time) / 1000;
                            if (dt > 0) {
                                data.network.rx_bps = (rx_bytes - last_rx_bytes) / dt;
                            }
                        }
                        last_rx_bytes = rx_bytes;
                    }
                }
                
                if (findValueInResponse(OID_IFOUTOCTETS, sizeof(OID_IFOUTOCTETS), &val, &val_len, &tag)) {
                    if (tag == 0x01 || tag == 0x41) {
                        uint32_t tx_bytes = decodeCounter(val, val_len);
                        if (last_net_time > 0) {
                            uint32_t dt = (now - last_net_time) / 1000;
                            if (dt > 0) {
                                data.network.tx_bps = (tx_bytes - last_tx_bytes) / dt;
                            }
                        }
                        last_tx_bytes = tx_bytes;
                    }
                }
                
                last_net_time = now;
                data.is_online = true;
            } else {
                Serial.println("[SNMP] Response timeout");
            }
            
            switch (last_query) {
                case SNMP_QUERY_SYSNAME: state = SNMP_QUERY_UPTIME; break;
                case SNMP_QUERY_UPTIME: state = SNMP_QUERY_CPU; break;
                case SNMP_QUERY_CPU: state = SNMP_QUERY_IF_IN; break;
                case SNMP_QUERY_IF_IN: state = SNMP_QUERY_IF_OUT; break;
                case SNMP_QUERY_IF_OUT: 
                    data.last_update_ms = now;
                    data.has_update = true;
                    state = SNMP_DONE;
                    Serial.println("[SNMP] Poll complete");
                    break;
                default: state = SNMP_QUERY_SYSNAME; break;
            }
            break;
            
        case SNMP_DONE:
            state = SNMP_QUERY_SYSNAME;
            last_poll_ms = now;
            return false;
    }
    
    last_poll_ms = now;
    return data.has_update;
}

bool SnmpClient::isConnected() { return data.is_online; }
const NasData& SnmpClient::getData() { return data; }
const char* SnmpClient::getTypeName() { return "SNMP"; }
const char* SnmpClient::getConnIcon() { return "wifi"; }
