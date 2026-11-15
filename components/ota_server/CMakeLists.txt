idf_component_register(SRCS "ota_server.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES app_state esp_http_server app_update esp_system esp_timer log freertos)
