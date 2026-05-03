// Mock Data Source for Testing
#pragma once
#include "data_source.h"

class MockDataSource : public DataSource {
public:
    MockDataSource();
    ~MockDataSource() override;
    bool init(Preferences& prefs) override;
    bool connect() override;
    bool poll() override;
    bool isConnected() override;
    const NasData& getData() override;
    const char* getTypeName() override;
    const char* getConnIcon() override;
    NasTypeConfig getConfig() override { return NasTypeConfig::getDefaults(NAS_MOCK); }

private:
    NasData data;
    uint32_t last_poll_ms;

    void generateMockData();
};
