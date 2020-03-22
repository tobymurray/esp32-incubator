#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASSWORD CONFIG_WIFI_PASSWORD
#define MAXIMUM_RETRY CONFIG_WIFI_MAXIMUM_RETRY

/* The event group allows multiple bits for each event, but we only care about one event
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static const char* TAG = "WiFi helper";

static int s_retry_num = 0;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

RTC_DATA_ATTR static useconds_t connection_delay = 500000;  // 1/2 second Delay in usec

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  // When starting Wi-Fi has gone well and the intention is to connect in station mode
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
    // If the Wi-Fi connection is disconnected unexpectedly (or fails to set up a connection in some cases)
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < MAXIMUM_RETRY) {
      esp_wifi_connect();
      xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    }
    ESP_LOGI(TAG, "connect to the AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
    ESP_LOGI(TAG, "Obtained IP address: %s", ip4addr_ntoa(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

/**
 * Starts process of connecting to Wi-Fi, but doesn't block
 */
void initialize_wifi_in_station_mode(void) {
  wifi_event_group = xEventGroupCreate();

  tcpip_adapter_init();

  ESP_ERROR_CHECK(esp_event_loop_create_default());

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

  wifi_config_t wifi_config = {
      .sta = {.ssid = WIFI_SSID, .password = WIFI_PASSWORD},
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "Connecting to Wi-Fi network: %s", WIFI_SSID);
  esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
}

/**
 * Block, waiting for IP address from Wi-Fi connection
 */
void wait_for_ip(void) {
  ESP_LOGI(TAG, "Waiting for IP address");
  EventBits_t uxBits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, pdMS_TO_TICKS(10000));

  if (uxBits && WIFI_CONNECTED_BIT) {
    return;
  }

  if (connection_delay <= (1000000LL * 1024)) {  // roughly 17 minutes
    // Wait twice as long as the time before
    connection_delay *= 2;
  }

  long long int minutes = (connection_delay / 1000000LL) / 60;
  long long int seconds = (connection_delay / 1000000LL) % 60;
  if (minutes > 0) {
    ESP_LOGI(TAG, "Failed to get an IP address, so going to sleep for %lld minutes and %lld seconds", minutes, seconds);
  } else {
    ESP_LOGI(TAG, "Failed to get an IP address, so going to sleep for %lld seconds", seconds);
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_deep_sleep(connection_delay);
  }
}
