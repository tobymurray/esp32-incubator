#include "chicken_incubator.h"
#include "heater.h"
#include "humidifier.h"
#include "bme280_helper.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "32700";

static const long MILLIS_PER_DAY = 1000 * 60 * 60 * 24;

enum heating_states {
  HEATING,
  COOLING
};

static enum heating_states heating_state = COOLING;

// static int MINIMUM_TEMPERATURE = 37;
// static int MAXIMUM_TEMPERATURE = 39;

// static int MINIMUM_HUMIDITY = 50;
// static int MAXIMUM_HUMIDITY = 60;

// This is the ideal temperature for the first 18 days
static float TARGET_INCUBATION_TEMPERATURE = 37.5;
// This is the ideal temperature for the last 3 days
static float TARGET_HATCHER_TEMPERATURE = 36.9;
// How much the temperature can vary above and below the target before the heater is turned on
static float TEMPERATURE_VARIANCE = 0.2;

// This is the ideal humidity for the first 18 days
static float TARGET_INCUBATION_HUMIDITY = 58;
// This is the ideal humidity for the last 3 days
static float TARGET_HATCHER_HUMIDITY = 70.5;
// How much the humidity can vary above and below the target before the humidifier is turned on
static float HUMIDITY_VARIANCE = 5;

void chicken_temperature_reading_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
  struct EventData * data = (struct EventData *) event_data;
  float temperature = data->reading;
  int i2c_address = data->sensor_address;
  ESP_LOGI(TAG, "Received temperature reading: %.2f*C from sensor %X", temperature, i2c_address);
  if (heating_state == COOLING && temperature < (TARGET_INCUBATION_TEMPERATURE - TEMPERATURE_VARIANCE)) {
    ESP_LOGI(TAG, "Temperature %.2f*C is below threshold %.2f*C, turning heater on", temperature, TARGET_INCUBATION_TEMPERATURE - TEMPERATURE_VARIANCE);
    turn_on_heater();
    heating_state = HEATING;
  } else if (heating_state == HEATING && temperature > (TARGET_INCUBATION_TEMPERATURE + TEMPERATURE_VARIANCE)) {
    ESP_LOGI(TAG, "Temperature %.2f*C is above threshold %.2f*C, turning heater off", temperature, TARGET_INCUBATION_TEMPERATURE + TEMPERATURE_VARIANCE);
    turn_off_heater();
    heating_state = COOLING;
  }
}

void chicken_humidity_reading_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
  struct EventData * data = (struct EventData *) event_data;
  float humidity = data->reading;
  // ESP_LOGI(TAG, "Received humidity reading: %.2f%%", humidity);
  if (humidity < (TARGET_INCUBATION_HUMIDITY - HUMIDITY_VARIANCE)) {
    // ESP_LOGI(TAG, "Humidity %.2f%% is below threshold %.2f%%, turning humidifier on", humidity, TARGET_INCUBATION_HUMIDITY - HUMIDITY_VARIANCE);
    // turn_on_humidifier();
  } else if (humidity > (TARGET_INCUBATION_HUMIDITY + HUMIDITY_VARIANCE)) {
    // ESP_LOGI(TAG, "Humidity %.2f%% is above threshold %.2f%%, turning humdifier off", humidity, (TARGET_INCUBATION_HUMIDITY + HUMIDITY_VARIANCE));
    turn_off_humidifier();
  }
}

static void egg_turner_callback(void* arg)
{
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "Periodic timer called, time since boot: %lld us", time_since_boot);
}

void chicken_start() {
    const esp_timer_create_args_t egg_turner_timer_args = {
            .callback = &egg_turner_callback,
            .name = "egg_turner"
    };

    esp_timer_handle_t egg_turner_timer;
    ESP_ERROR_CHECK(esp_timer_create(&egg_turner_timer_args, &egg_turner_timer));

    ESP_ERROR_CHECK(esp_timer_start_periodic(egg_turner_timer, MILLIS_PER_DAY/5));
}