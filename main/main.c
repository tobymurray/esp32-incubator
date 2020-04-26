#include "bme280_helper.h"
#include "chicken_incubator.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "heater.h"
#include "humidifier.h"
#include "nvs_flash.h"
#include "sntp_helper.h"
#include "uln2003_stepper_driver.h"
#include "wifi_helper.h"

static const char* TAG = "Main";

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;

static void initialize(void) {
  // Quiet a bunch of logs I'm not interested in right now
  esp_log_level_set("boot", ESP_LOG_WARN);
  esp_log_level_set("heap_init", ESP_LOG_WARN);
  esp_log_level_set("esp_image", ESP_LOG_WARN);
  esp_log_level_set("spi_flash", ESP_LOG_WARN);
  esp_log_level_set("system_api", ESP_LOG_WARN);
  esp_log_level_set("wifi", ESP_LOG_WARN);
  esp_log_level_set("phy", ESP_LOG_WARN);
  esp_log_level_set("tcpip_adapter", ESP_LOG_WARN);

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Create the default event loop
  ESP_ERROR_CHECK(esp_event_loop_create_default());
}

void app_main(void) {
  ++boot_count;
  ESP_LOGI(TAG, "Boot count: %d", boot_count);
  initialize();
  initialize_heater();
  initialize_humidifier();

  ESP_ERROR_CHECK(esp_event_handler_register(SENSOR_EVENTS, SENSOR_READING_TEMPERATURE, chicken_temperature_reading_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(SENSOR_EVENTS, SENSOR_READING_HUMIDITY, chicken_humidity_reading_handler, NULL));

  // initialize_wifi_in_station_mode();
  // wait_for_ip();

  // time_t now;
  // set_current_time(&now);

  // if (!time_is_set(now) || time_is_stale(now)) {
  //   ESP_LOGI(TAG, "Time has either not been set or become stale. Connecting to WiFi and syncing time over NTP.");
  //   wait_for_ip();
  //   obtain_time(&now);
  // }

  // char strftime_buf[64];
  // get_time_string(strftime_buf);
  // ESP_LOGI(TAG, "Time is: %s", strftime_buf);

  start_bme280_read_tasks();
}
