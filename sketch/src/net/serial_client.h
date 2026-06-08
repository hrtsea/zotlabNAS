// ZotLab NAS Monitor - Serial Client (UART Linux Serial mode)
#pragma once

#include "data_source.h"
#include <HardwareSerial.h>
#include <ArduinoJson.h>

enum SerialState {
    SERIAL_IDLE,
    SERIAL_WAITING_FRAME,
    SERIAL_RECEIVING,
    SERIAL_PARSING,
    SERIAL_DONE
};

class SerialClient : public DataSource {
public:
    SerialClient();
    ~SerialClient() override;

    bool init(Preferences& prefs) override;
    bool connect() override;
    bool poll() override;
    bool isConnected() override;
    const NasData& getData() override;
    const char* getTypeName() override;
    const char* getConnIcon() override;
    NasTypeConfig getConfig() override { return NasTypeConfig::getDefaults(NET_LINUX_SERIAL); }

private:
    HardwareSerial* serial;
    uint32_t baud_rate;
    SerialState state;
    uint32_t last_frame_ms;
    char rx_buffer[SERIAL_BUF_SIZE];
    int rx_pos;
    NasData data;
    bool connected;

    bool readFrame();
    bool parseJSON(const char* json);
    void clearData();
};
