#include "heater.h"
#include "common.h"

#include "driver/gpio.h"
#include "esp_log.h"

#define GPIO_PIN CONFIG_GPIO_NUMBER
#define OFF 0
#define ON 1

static const char *TAG = "heater";

void initialize_heater() {
  ESP_LOGI(TAG, "Initializing heater control on pin %d", GPIO_PIN);
  pinModeOutput(GPIO_PIN);
  gpio_set_level(GPIO_PIN, OFF);
}

void turn_on_heater() {
  ESP_LOGI(TAG, "Turning heater on");
  gpio_set_level(GPIO_PIN, ON);
}

void turn_off_heater() {
  ESP_LOGI(TAG, "Turning heater off");
  gpio_set_level(GPIO_PIN, OFF);
}
