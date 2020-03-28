#include "humidifier.h"
#include "common.h"

#include "driver/gpio.h"
#include "esp_log.h"

#define GPIO_PIN CONFIG_HUMIDIFIER_GPIO_NUMBER
#define OFF 1
#define ON 0

static const char *TAG = "humidifier";

void initialize_humidifier() {
  ESP_LOGI(TAG, "Initializing humidifier control on pin %d", GPIO_PIN);
  pinModeOutput(GPIO_PIN);
  gpio_set_level(GPIO_PIN, OFF);
}

void turn_on_humidifier() {
  ESP_LOGI(TAG, "Turning humidifier on");
  gpio_set_level(GPIO_PIN, ON);
}

void turn_off_humidifier() {
  ESP_LOGI(TAG, "Turning humidifier off");
  gpio_set_level(GPIO_PIN, OFF);
}
