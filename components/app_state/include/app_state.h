#pragma once

#include <stdbool.h>

/*
 * Єдине джерело правди про стан пристрою.
 * Стани згруповані за тим, де розірвався ланцюжок «плата → Wi-Fi → інтернет → сервер».
 */
typedef enum {
    APP_STATE_BOOT = 0,      // старт, ще не було жодного результату
    APP_STATE_OK,            // останній пінг — 2xx
    APP_STATE_NO_WIFI,       // немає звʼязку з роутером
    APP_STATE_NO_INTERNET,   // Wi-Fi є, але сервер недосяжний (DNS/TCP/TLS/таймаут) або 5xx
    APP_STATE_REJECTED,      // сервер відповів відмовою (4xx: неправильний ключ, 429, ...)
    APP_STATE_OTA,           // іде оновлення прошивки (накладається поверх інших станів)
} app_state_t;

typedef void (*app_state_listener_t)(app_state_t state);

void app_state_init(app_state_listener_t listener);

/** Записати стан звʼязку. Слухач отримає подію лише якщо видимий стан змінився. */
void app_state_set(app_state_t state);

/** Увімкнути/вимкнути режим OTA. Поки він активний, видимий стан — APP_STATE_OTA. */
void app_state_set_ota(bool active);

/** Видимий стан (з урахуванням OTA). */
app_state_t app_state_get(void);

const char *app_state_name(app_state_t state);
