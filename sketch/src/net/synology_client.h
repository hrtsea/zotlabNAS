// Synology DSM API Client
#pragma once
#include "data_source.h"

class SynologyClient : public DataSource {
public:
    SynologyClient();
    ~SynologyClient() override;
    bool init(Preferences& prefs) override;
    bool connect() override;
    bool poll() override;
    bool isConnected() override;
    const NasData& getData() override;
    const char* getTypeName() override;
    const char* getConnIcon() override;
    NasTypeConfig getConfig() override { return NasTypeConfig::getDefaults(NAS_SYNOLOGY); }

private:
    char nas_ip[40];
    uint16_t nas_port;
    char username[32];
    char password[64];
    char sid[64];
    bool use_https;
    NasData data;
    uint32_t last_poll_ms;
    uint8_t consecutive_failures;  // 连续失败次数,用于退避机制

    bool login();
    bool fetchSystem();
    bool fetchStorage();
};
