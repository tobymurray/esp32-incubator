#ifndef chicken_incubator_h
#define chicken_incubator_h

#include "esp_event.h"

void chicken_temperature_reading_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
void chicken_humidity_reading_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);

#endif
