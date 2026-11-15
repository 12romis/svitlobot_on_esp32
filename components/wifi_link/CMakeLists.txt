idf_component_register(SRCS "wifi_link.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES app_state esp_wifi esp_netif esp_event esp_timer log freertos)
