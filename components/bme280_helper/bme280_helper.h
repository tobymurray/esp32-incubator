#ifndef BME280_HELPER_H_
#define BME280_HELPER_H_

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(SENSOR_EVENTS);
enum {
    SENSOR_READING_TEMPERATURE,
    SENSOR_READING_HUMIDITY
};

struct EventData {
  float reading;
  int sensor_address;
};

void start_bme280_read_tasks(void);

#endif