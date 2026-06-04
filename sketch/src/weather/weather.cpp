#include "weather.h"
#include "net/network.h"
#include "pcf85063/pcf85063.h"
#include "ui/ui.h"
#include <Arduino.h>
#include <ArduinoJson.h>

const char *weatherUrl = "http://api.weatherapi.com/v1/current.json?key=%s&q=%s&aqi=yes";
const char *weatherApiKey = "1f00c88f3483483a9ba62101250612";

char query_parameter[64] = "auto:ip"; 
const int8_t UTC_offset_hour[27] = {14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12};
const int8_t UTC_offset_minute[7] = {45, 30, 15, 0, -15, -30, -45};
uint8_t temp_unit = 0; 
uint8_t city_index = 0;
uint8_t offset_hour_index = 0;
uint8_t offset_minute_index = 0;

static TaskHandle_t weatherTask = NULL;

struct Weather_JSON { 
  char state[64];
  uint8_t usepa_index;
  const char *name;
  char last_updated[32];
  float temp_c;
  float temp_f;
  float wind_kph;
  float wind_mph;
  float wind_degree;
  char wind_dir[8];
  float pressure_mb;
  float pressure_in;
  float precip_mm;
  float precip_in;
  float humidity;
  float cloud;
  float feelslike_c;
  float feelslike_f;
  float uv;
  uint16_t code;
  uint8_t is_day;
  float co;
  float no2;
  float o3;
  float so2;
  float pm2_5;
  float pm10;
};

const int us_epa_index_colors[6] = {
    0x00E400, 
    0xFFD700, 
    0xFF7E00, 
    0xff0000, 
    0x8F3F97, 
    0x7E0023
};
const char *us_epa_index_names[6] = {"Good", "Moderate", "Unhealthy for Sensitive Groups", 
                                      "Unhealthy", "Very Unhealthy", "Hazardous"};

char USAQI[8]; 
char dominantPollutant[64]; 
void parseAQI(float co, float no2, float o3, float so2, float pm2_5, float pm10) {
  char buffer[64];
  snprintf(dominantPollutant, sizeof(dominantPollutant), "PM2.5 | %.1f ug/m3", pm2_5);
  snprintf(USAQI, sizeof(USAQI), "50");
}

void updateWeatherPanelTask(void *parameter) {
  log_i("fetch weather data");
  weatherTask = NULL;
  vTaskDelete(NULL);
}

void updateWeatherPanel() {
  if (weatherTask == NULL) 
    xTaskCreatePinnedToCore(updateWeatherPanelTask, "To update weather panel", 6 * 1024, NULL, 3, &weatherTask, 1);
}

void weatherAnimation(bool animate) {
}