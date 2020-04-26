idf_component_register(SRCS "chicken_incubator.c"
                  INCLUDE_DIRS "."
                  REQUIRES heater humidifier uln2003_stepper_driver bme280_helper mqtt_helper sntp_helper
                  )