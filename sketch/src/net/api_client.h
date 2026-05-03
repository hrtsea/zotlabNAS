// ZotLab NAS Monitor - HTTP API Client (Linux HTTP / Windows)
#pragma once

#include "data_source.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

enum ApiState {
    API_IDLE,
    API_REQUESTING,
    API_PARSING,
    API_DONE
};

class ApiClient : public DataSource {
public:
    ApiClient();
    ~ApiClient() override;

    bool init(Preferences& prefs) override;
    bool connect() override;
    bool poll() override;
    bool isConnected() override;
    const NasData& getData() override;
    const char* getTypeName() override;
    const char* getConnIcon() override;
    NasTypeConfig getConfig() override;

    // 设置当前类型（用于 getConfig 返回正确配置）
    void setType(NasType type);

private:
    NasType current_type;  // 当前类型（用于 getConfig）
    char nas_ip[40];
    uint16_t nas_port;
    ApiState state;
    uint32_t last_poll_ms;
    uint32_t poll_interval_ms;
    HTTPClient* http;
    NasData data;
    uint8_t consecutive_failures;  // 连续失败次数,用于退避机制

    bool fetchAll();
    bool parseJSON(const char* json);
    void clearData();
};
