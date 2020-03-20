#include "bme280_helper.h"

#include "bme280.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/task.h"

#define SDA_PIN 23
#define SCL_PIN 22

static const char *TAG = "32700";

void i2c_master_init() {
  i2c_config_t i2c_config = {.mode = I2C_MODE_MASTER,
                             .sda_io_num = SDA_PIN,
                             .scl_io_num = SCL_PIN,
                             .sda_pullup_en = GPIO_PULLUP_ENABLE,
                             .scl_pullup_en = GPIO_PULLUP_ENABLE,
                             .master.clk_speed = 1000000};
  i2c_param_config(I2C_NUM_0, &i2c_config);
  i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

char BME280_I2C_bus_write(unsigned char dev_addr, unsigned char reg_addr, unsigned char *reg_data, unsigned char cnt) {
  signed int iError = BME280_INIT_VALUE;

  esp_err_t espRc;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);

  i2c_master_write_byte(cmd, reg_addr, true);
  i2c_master_write(cmd, reg_data, cnt, true);
  i2c_master_stop(cmd);

  espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_PERIOD_MS);
  if (espRc == ESP_OK) {
    iError = SUCCESS;
  } else {
    iError = FAIL;
  }
  i2c_cmd_link_delete(cmd);

  return (s8)iError;
}

char BME280_I2C_bus_read(unsigned char dev_addr, unsigned char reg_addr, unsigned char *reg_data, unsigned char cnt) {
  signed int iError = BME280_INIT_VALUE;
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
    iError = SUCCESS;
  } else {
    iError = FAIL;
  }

  i2c_cmd_link_delete(cmd);

  return (s8)iError;
}

void BME280_delay_msek(unsigned int msek) { vTaskDelay(msek / portTICK_PERIOD_MS); }

void task_bme280_forced_mode(void *ignore) {
  struct bme280_t bme280 = {.bus_write = BME280_I2C_bus_write,
                            .bus_read = BME280_I2C_bus_read,
                            .dev_addr = BME280_I2C_ADDRESS2,
                            .delay_msec = BME280_delay_msek};

  signed int result;
  signed int v_uncomp_pressure;
  signed int v_uncomp_temperature;
  signed int v_uncomp_humidity;
  unsigned char wait_time;

  result = bme280_init(&bme280);
  if (result != SUCCESS) {
    ESP_LOGE(TAG, "Error while initializing. Code: %d", result);
    vTaskDelete(NULL);
  }

  result += bme280_set_oversamp_pressure(BME280_OVERSAMP_1X);
  result += bme280_set_oversamp_temperature(BME280_OVERSAMP_1X);
  result += bme280_set_oversamp_humidity(BME280_OVERSAMP_1X);

  result += bme280_set_filter(BME280_FILTER_COEFF_OFF);
  if (result != SUCCESS) {
    ESP_LOGE(TAG, "Error while setting configuration. Code: %d", result);
    vTaskDelete(NULL);
  }

  wait_time = bme280_compute_wait_time(&wait_time);
  while (true) {
    // bme280.delay_msec(wait_time);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    result = bme280_get_forced_uncomp_pressure_temperature_humidity(&v_uncomp_pressure, &v_uncomp_temperature,
                                                                    &v_uncomp_humidity);

    if (result == SUCCESS) {
      ESP_LOGI(TAG, "%.2f degC / %.3f hPa / %.3f %%", bme280_compensate_temperature_double(v_uncomp_temperature),
               bme280_compensate_pressure_double(v_uncomp_pressure) / 100,  // Pa -> hPa
               bme280_compensate_humidity_double(v_uncomp_humidity));
    } else {
      ESP_LOGE(TAG, "measure error. code: %d", result);
    }
  }

  vTaskDelete(NULL);
}

void kick_off(void) {
  i2c_master_init();
  xTaskCreate(&task_bme280_forced_mode, "bme280_forced_mode", 2048, NULL, 6, NULL);
}
