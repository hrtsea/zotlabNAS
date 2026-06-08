// QNAP QTS API Client
#pragma once
#include "data_source.h"
#include <HTTPClient.h>

enum QnapState {
    QNAP_IDLE,
    QNAP_LOGIN,
    QNAP_FETCH_SYSINFO,
    QNAP_FETCH_STORAGE,
    QNAP_FETCH_DISKS,
    QNAP_FETCH_NETWORK,
    QNAP_DONE
};

class QnapClient : public DataSource {
public:
    QnapClient();
    ~QnapClient() override;
    bool init(Preferences& prefs) override;
    bool connect() override;
    bool poll() override;
    bool isConnected() override;
    const NasData& getData() override;
    const char* getTypeName() override;
    const char* getConnIcon() override;
    NasTypeConfig getConfig() override { return NasTypeConfig::getDefaults(NAS_QNAP); }

private:
    HTTPClient* http;
    char nas_ip[40];
    uint16_t nas_port;
    char username[32];
    char password[64];
    char sid[64];
    NasData data;
    uint32_t last_poll_ms;
    QnapState state;
    uint8_t consecutive_failures;  // 连续失败次数,用于退避机制

    bool login();
    bool fetchSysInfo();
    bool fetchStorage();
    bool fetchDisks();
    bool fetchNetwork();
    const char* extractXML(const char* xml, const char* tag, int index = 0);
};
