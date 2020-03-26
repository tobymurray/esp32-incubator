#include "common.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"

void pinModeOutput(uint8_t pin) {
  gpio_config_t io_conf;
  // disable interrupt
  io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
  // set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  // bit mask of the pins that you want to set,e.g.GPIO18/19
  io_conf.pin_bit_mask = (1ULL << pin);
  // disable pull-down mode
  io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
  // disable pull-up mode
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  // configure GPIO with the given settings
  gpio_config(&io_conf);
}

unsigned long IRAM_ATTR millis() { return (unsigned long)(esp_timer_get_time() / 1000ULL); }

unsigned long IRAM_ATTR micros() { return (unsigned long)(esp_timer_get_time()); }

void IRAM_ATTR delayMicroseconds(uint32_t us) {
  uint32_t m = micros();
  if (us) {
    uint32_t e = (m + us);
    if (m > e) {  // overflow
      while (micros() > e) {
        NOP();
      }
    }
    while (micros() < e) {
      NOP();
    }
  }
}

void delay(uint32_t ms) { vTaskDelay(ms / portTICK_PERIOD_MS); }
