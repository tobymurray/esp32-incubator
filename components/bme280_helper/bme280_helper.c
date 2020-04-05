#include "bme280_helper.h"

#include "bme280.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SDA_PIN 23
#define SCL_PIN 22

static const char *TAG = "BME280_HELPER";

ESP_EVENT_DEFINE_BASE(SENSOR_EVENTS);

void i2c_master_init() {
  i2c_config_t i2c_config = {.mode = I2C_MODE_MASTER,
                             .sda_io_num = SDA_PIN,
                             .scl_io_num = SCL_PIN,
                             .sda_pullup_en = GPIO_PULLUP_ENABLE,
                             .scl_pullup_en = GPIO_PULLUP_ENABLE,
                             .master.clk_speed = 50000};
  i2c_param_config(I2C_NUM_0, &i2c_config);
  i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

int8_t BME280_I2C_bus_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint16_t cnt) {
  signed int iError = BME280_OK;

  esp_err_t espRc;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);

  i2c_master_write_byte(cmd, reg_addr, true);
  i2c_master_write(cmd, reg_data, cnt, true);
  i2c_master_stop(cmd);

  espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_PERIOD_MS);
  if (espRc == ESP_OK) {
    iError = BME280_OK;
  } else {
    iError = BME280_E_COMM_FAIL;
  }
  i2c_cmd_link_delete(cmd);

  return (int8_t )iError;
}

int8_t BME280_I2C_bus_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint16_t cnt) {
  signed int iError = BME280_OK;
  esp_err_t espRc;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg_addr, true);

  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);

  if (cnt > 1) {
    i2c_master_read(cmd, reg_data, cnt - 1, I2C_MASTER_ACK);
  }
  i2c_master_read_byte(cmd, reg_data + cnt - 1, I2C_MASTER_NACK);
  i2c_master_stop(cmd);

  espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_PERIOD_MS);
  if (espRc == ESP_OK) {
    iError = BME280_OK;
  } else {
    iError = BME280_E_COMM_FAIL;
  }

  i2c_cmd_link_delete(cmd);

  return (int8_t )iError;
}

void BME280_delay_msek(unsigned int msek) { vTaskDelay(msek / portTICK_PERIOD_MS); }

void task_bme280_forced_mode(void *i2c_address) {
  struct bme280_dev bme280 = {
    .intf = BME280_I2C_INTF,
    .dev_id = *(uint8_t *)i2c_address,
    .read = BME280_I2C_bus_read,
    .write = BME280_I2C_bus_write,
    .delay_ms = BME280_delay_msek
  };

  /* Variable to define the result */
  int8_t rslt = BME280_OK;

  /* Variable to define the selecting sensors */
  uint8_t settings_sel = 0;

  /* Variable to store minimum wait time between consecutive measurement in force mode */
  uint32_t req_delay;

  /* Structure to get the pressure, temperature and humidity values */
  struct bme280_data comp_data;

  /* Recommended mode of operation: Indoor navigation */
  bme280.settings.osr_h = BME280_OVERSAMPLING_1X;
  bme280.settings.osr_p = BME280_OVERSAMPLING_16X;
  bme280.settings.osr_t = BME280_OVERSAMPLING_2X;
  bme280.settings.filter = BME280_FILTER_COEFF_16;

  settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

  /* Set the sensor settings */
  rslt = bme280_set_sensor_settings(settings_sel, &bme280);
  if (rslt != BME280_OK) {
    ESP_LOGE(TAG, "Failed to set sensor settings (code %+d).", rslt);
    vTaskDelete(NULL);
  }

  /*Calculate the minimum delay required between consecutive measurement based upon the sensor enabled
    *  and the oversampling configuration. */
  req_delay = bme280_cal_meas_delay(&bme280.settings);

  /* Continuously stream sensor data */
  while (true)  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    /* Set the sensor to forced mode */
    rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, &bme280);
    if (rslt != BME280_OK) {
        ESP_LOGE(TAG, "Failed to set sensor mode (code %+d).", rslt);
      vTaskDelete(NULL);
    }

    /* Wait for the measurement to complete and print data */
    bme280.delay_ms(req_delay);
    rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, &bme280);
    if (rslt != BME280_OK) {
      ESP_LOGE(TAG, "Failed to get sensor data (code %+d).", rslt);
      vTaskDelete(NULL);
    }

    float temp = comp_data.temperature;
    float hum = comp_data.humidity;
    ESP_LOGI(TAG, "Address %#x, %.2f degC / %.3f %%", bme280.dev_id, temp, hum);

    struct EventData event_data;
    event_data.sensor_address = bme280.dev_id;

    event_data.reading = temp;
    ESP_ERROR_CHECK(esp_event_post(SENSOR_EVENTS, SENSOR_READING_TEMPERATURE, &event_data, sizeof(event_data), portMAX_DELAY));

    event_data.reading = hum;
    ESP_ERROR_CHECK(esp_event_post(SENSOR_EVENTS, SENSOR_READING_HUMIDITY, &event_data, sizeof(event_data), portMAX_DELAY));
  }
  
  vTaskDelete(NULL);
}

void start_bme280_read_tasks(void) {
  i2c_master_init();
  uint8_t i2c_address_1 = BME280_I2C_ADDR_PRIM;
  xTaskCreate(&task_bme280_forced_mode, "bme280_forced_mode_primary", 2048, (void *)&i2c_address_1, 6, NULL);

  // Offset the second task so they happen at different times
  uint8_t i2c_address_2 = BME280_I2C_ADDR_SEC;
  vTaskDelay(500 / portTICK_PERIOD_MS);
  xTaskCreate(&task_bme280_forced_mode, "bme280_forced_mode_secondary", 2048, (void *)&i2c_address_2, 6, NULL);
}
