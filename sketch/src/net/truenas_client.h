// TrueNAS REST API Client
#pragma once
#include "data_source.h"
#include <HTTPClient.h>

enum TrueNASState {
    TRUENAS_IDLE,
    TRUENAS_FETCH_SYSTEM,
    TRUENAS_FETCH_POOLS,
    TRUENAS_FETCH_DISKS,
    TRUENAS_FETCH_INTERFACES,
    TRUENAS_DONE
};

class TrueNASClient : public DataSource {
public:
    TrueNASClient();
    ~TrueNASClient() override;
    bool init(Preferences& prefs) override;
    bool connect() override;
    bool poll() override;
    bool isConnected() override;
    const NasData& getData() override;
    const char* getTypeName() override;
    const char* getConnIcon() override;
    NasTypeConfig getConfig() override { return NasTypeConfig::getDefaults(NAS_TRUENAS); }

private:
    HTTPClient* http;
    char nas_ip[40];
    uint16_t nas_port;
    char api_key[128];
    NasData data;
    uint32_t last_poll_ms;
    TrueNASState state;
    uint8_t consecutive_failures;  // 连续失败次数,用于退避机制

    bool fetchSystem();
    bool fetchPools();
    bool fetchDisks();
    bool fetchInterfaces();
};
