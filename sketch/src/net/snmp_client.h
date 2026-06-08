// SNMP v2c Client
#pragma once
#include "data_source.h"
#include <WiFiUdp.h>

enum SnmpState {
    SNMP_IDLE,
    SNMP_QUERY_SYSNAME,
    SNMP_QUERY_UPTIME,
    SNMP_QUERY_CPU,
    SNMP_QUERY_IF_IN,
    SNMP_QUERY_IF_OUT,
    SNMP_WAIT_RESPONSE,
    SNMP_CALC_RATES,
    SNMP_DONE
};

class SnmpClient : public DataSource {
public:
    SnmpClient();
    ~SnmpClient() override;
    bool init(Preferences& prefs) override;
    bool connect() override;
    bool poll() override;
    bool isConnected() override;
    const NasData& getData() override;
    const char* getTypeName() override;
    const char* getConnIcon() override;
    NasTypeConfig getConfig() override { return NasTypeConfig::getDefaults(NET_SNMP); }

private:
    WiFiUDP* udp;
    char nas_ip[40];
    uint16_t nas_port;
    char community[32];
    NasData data;
    uint32_t last_poll_ms;
    uint32_t last_rx_bytes;
    uint32_t last_tx_bytes;
    uint32_t last_net_time;
    
    SnmpState state;
    SnmpState last_query;
    uint16_t request_id;
    uint8_t rx_buf[512];
    int rx_len;

    bool sendGet(const uint8_t* oid, size_t oid_len);
    bool waitForResponse(uint32_t timeout_ms);
    int decodeInteger(const uint8_t* buf, size_t len);
    size_t decodeLength(const uint8_t* buf, size_t* out_len);
    bool decodeOctetString(const uint8_t* buf, size_t len, char* out, size_t out_size);
    uint32_t decodeCounter(const uint8_t* buf, size_t len);
    bool findValueInResponse(const uint8_t* target_oid, size_t oid_len, 
                             const uint8_t** value_out, size_t* value_len, uint8_t* tag_out);
};
