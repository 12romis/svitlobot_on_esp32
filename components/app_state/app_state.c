#include "app_state.h"

#include "freertos/FreeRTOS.h"
#include "esp_log.h"

static const char *TAG = "app_state";

static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static app_state_t s_link_state = APP_STATE_BOOT;
static bool s_ota_active;
static app_state_t s_visible = APP_STATE_BOOT;
static app_state_listener_t s_listener;

static app_state_t visible_locked(void)
{
    return s_ota_active ? APP_STATE_OTA : s_link_state;
}

// Перераховує видимий стан і, якщо він змінився, сповіщає слухача (поза критичною секцією).
static void publish(void)
{
    taskENTER_CRITICAL(&s_lock);
    app_state_t next = visible_locked();
    bool changed = next != s_visible;
    app_state_t prev = s_visible;
    s_visible = next;
    taskEXIT_CRITICAL(&s_lock);

    if (!changed) {
        return;
    }
    ESP_LOGI(TAG, "%s -> %s", app_state_name(prev), app_state_name(next));
    if (s_listener) {
        s_listener(next);
    }
}

void app_state_init(app_state_listener_t listener)
{
    s_listener = listener;
    if (s_listener) {
        s_listener(app_state_get());
    }
}

void app_state_set(app_state_t state)
{
    taskENTER_CRITICAL(&s_lock);
    s_link_state = state;
    taskEXIT_CRITICAL(&s_lock);
    publish();
}

void app_state_set_ota(bool active)
{
    taskENTER_CRITICAL(&s_lock);
    s_ota_active = active;
    taskEXIT_CRITICAL(&s_lock);
    publish();
}

app_state_t app_state_get(void)
{
    taskENTER_CRITICAL(&s_lock);
    app_state_t state = s_visible;
    taskEXIT_CRITICAL(&s_lock);
    return state;
}

const char *app_state_name(app_state_t state)
{
    switch (state) {
    case APP_STATE_BOOT:        return "BOOT";
    case APP_STATE_OK:          return "OK";
    case APP_STATE_NO_WIFI:     return "NO_WIFI";
    case APP_STATE_NO_INTERNET: return "NO_INTERNET";
    case APP_STATE_REJECTED:    return "REJECTED";
    case APP_STATE_OTA:         return "OTA";
    }
    return "?";
}
