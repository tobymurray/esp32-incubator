#define LOW 0
#define HIGH 1

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "common.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "uln2003_stepper_driver.h"

static const char *TAG = "stepper";

#define IN1_PIN CONFIG_IN1_PIN
#define IN2_PIN CONFIG_IN2_PIN
#define IN3_PIN CONFIG_IN3_PIN
#define IN4_PIN CONFIG_IN4_PIN

int pins[4] = {IN1_PIN, IN2_PIN, IN3_PIN, IN4_PIN};

int steps[8][4] = {
	  {LOW, HIGH, HIGH, HIGH},
	  {LOW, LOW, HIGH, HIGH},
	  {HIGH, LOW, HIGH, HIGH},
	  {HIGH, LOW, LOW, HIGH},
	  {HIGH, HIGH, LOW, HIGH},
	  {HIGH, HIGH, LOW, LOW},
	  {HIGH, HIGH, HIGH, LOW},
	  {LOW, HIGH, HIGH, LOW}
	};

void set_up() {
  for (int i = 0; i < 4; i++) {
    ESP_LOGI(TAG, "Setting pin %d (%d) to output", i, pins[i]);
    pinModeOutput(pins[i]);
  }
}

void rotate() {
  ESP_LOGI(TAG, "Starting steps...");
  for (int i = 0; i < 1000; i++) {
    for (int step = 0; step < 8; step++) {
      vTaskDelay(10 / portTICK_PERIOD_MS);
      for (int pin = 0; pin < 4; pin++) {
        delayMicroseconds(100);
        gpio_set_level(pins[pin], steps[step][pin]);
      }
    }
  }
  ESP_LOGI(TAG, "...Steps done");
}
