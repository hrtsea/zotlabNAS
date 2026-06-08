// Netdata REST API Client
#pragma once
#include "data_source.h"
#include <HTTPClient.h>

enum NetdataState {
    NETDATA_IDLE,
    NETDATA_FETCH_CPU,
    NETDATA_FETCH_RAM,
    NETDATA_FETCH_NETWORK,
    NETDATA_FETCH_STORAGE,
    NETDATA_DONE
};

class NetdataClient : public DataSource {
public:
    NetdataClient();
    ~NetdataClient() override;
    bool init(Preferences& prefs) override;
    bool connect() override;
    bool poll() override;
    bool isConnected() override;
    const NasData& getData() override;
    const char* getTypeName() override;
    const char* getConnIcon() override;
    NasTypeConfig getConfig() override { return NasTypeConfig::getDefaults(NET_NETDATA); }

private:
    HTTPClient* http;
    char nas_ip[40];
    uint16_t nas_port;
    NasData data;
    uint32_t last_poll_ms;
    NetdataState state;
    uint8_t consecutive_failures;  // 连续失败次数,用于退避机制

    bool fetchCPU();
    bool fetchRAM();
    bool fetchNetwork();
    bool fetchStorage();
};
