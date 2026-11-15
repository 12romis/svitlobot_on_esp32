idf_component_register(SRCS "app_main.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES app_state status_led wifi_link pinger ota_server
                                     nvs_flash esp_netif esp_event esp_system log)
