#pragma once

#include "esp_err.h"

/**
 * HTTP-сервер у локальній мережі для оновлення прошивки:
 *   curl --data-binary @build/svitlobot.bin -H "X-OTA-Token: <токен>" http://svitlobot.local/ota
 * Також GET /status — стан, версія, аптайм (для діагностики без серійного порту).
 * Після запису плата перезавантажується; нова прошивка підтверджується в pinger,
 * інакше bootloader відкотить її на попередню.
 */
esp_err_t ota_server_start(const char *token);
