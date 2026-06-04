// Handle message que between task AUDIO and UI

#include "task_msg.h"
#include "esp32-hal.h"
#include "esp_err.h"

#include "pcf85063/pcf85063.h"
#include "ui/ui.h"
#include "weather/weather.h"
#include <LittleFS.h>
#include "net/network.h"
#include "ESP32-audioI2S-master/Audio.h"

// 外部变量声明
extern Audio audio;

bool seeking_now = false;

//###########  MESSAGE QUEUE SENDER functions ###############################

void updateSDCARDStatus(const char *status,  const uint32_t wifiColor) {
}

void audioSetAudioFilePosition(uint16_t pos) {
}

void audioSetPlayTime(uint16_t pos) {
}

void audioStopSong() {
}

void audioPauseResume() {
}

void audioSetTimeOffset(int offset) {
}

void audioSetVolume(int volume) {
}

void audioPlayFS(uint8_t source, const char *filename) {
}

void audioPlayHOST(const char *filename, const char *stationName) {
}

//----------  Clock display helper functions ------------------------
void nixie_clock(uint8_t hour, uint8_t minute, uint8_t second) {
}

//###########  MESSAGE QUEUE RECEIVER functions ###############################

void process_ui_status_queue() {
}

void process_audio_cmd_que() {
}