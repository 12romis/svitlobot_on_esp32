#pragma once

#include <stdint.h>
#include "esp_err.h"

typedef struct {
    const char *url;          // напр. https://api.svitlobot.in.ua/channelPing
    const char *channel_key;  // ключ каналу з чат-бота Світлобота
    uint32_t interval_s;      // період пінгу
    uint32_t timeout_ms;      // таймаут HTTP-операцій
} pinger_config_t;

/**
 * Запускає задачу пінгу. Задача підписана на Task Watchdog: якщо цикл зависне,
 * плата перезавантажиться. Результат кожного пінгу записується в app_state.
 */
esp_err_t pinger_start(const pinger_config_t *cfg);

/** Пінгнути позачергово (напр. одразу після відновлення Wi-Fi). Безпечно з будь-якої задачі. */
void pinger_wake(void);
