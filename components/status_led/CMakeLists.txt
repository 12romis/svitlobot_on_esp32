idf_component_register(SRCS "status_led.c"
                       INCLUDE_DIRS "include"
                       REQUIRES app_state
                       PRIV_REQUIRES esp_driver_gpio esp_timer log freertos)
