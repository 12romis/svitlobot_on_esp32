idf_component_register(SRCS "pinger.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES app_state wifi_link esp_http_client mbedtls app_update esp_system esp_timer log freertos)
