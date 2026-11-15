#pragma once

#include <stdbool.h>
#include "esp_err.h"

typedef void (*wifi_link_connected_cb_t)(void);

typedef struct {
    const char *ssid;
    const char *password;
    const char *hostname;
    wifi_link_connected_cb_t on_connected; // викликається після отримання IP
} wifi_link_config_t;

/**
 * Запускає Wi-Fi у режимі станції. Підключення й перепідключення — подієві,
 * з backoff 1 → 2 → 4 … 60 с. При втраті звʼязку ставить APP_STATE_NO_WIFI.
 * Потребує esp_netif_init() і esp_event_loop_create_default().
 */
esp_err_t wifi_link_start(const wifi_link_config_t *cfg);

bool wifi_link_is_connected(void);

/** RSSI поточної точки доступу в dBm або 0, якщо не підключено. */
int wifi_link_rssi(void);
