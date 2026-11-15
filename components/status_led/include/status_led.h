#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "app_state.h"

/*
 * Нескінченні патерни одного світлодіода — по одному на кожен стан.
 * Патерн крутиться, доки стан не зміниться, тому його неможливо «пропустити».
 * Рівного світла немає навіть у OK: коротке згасання раз на 3 с показує, що прошивка жива.
 */
esp_err_t status_led_init(int gpio, bool active_high);

/** Перемкнути патерн. Сумісна з app_state_listener_t. */
void status_led_show(app_state_t state);
