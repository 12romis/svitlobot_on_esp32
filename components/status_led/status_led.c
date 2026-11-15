#include "status_led.h"

#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "status_led";

// Тривалості кроків у мс, по черзі: увімкнено, вимкнено, увімкнено, ... Патерн повторюється.
typedef struct {
    const uint16_t *steps;
    size_t count;
} pattern_t;

#define PATTERN(...) { (const uint16_t[]){ __VA_ARGS__ }, sizeof((uint16_t[]){ __VA_ARGS__ }) / sizeof(uint16_t) }

static const pattern_t PATTERNS[] = {
    // Старт / OTA: швидке мерехтіння 5 Гц
    [APP_STATE_BOOT]        = PATTERN(100, 100),
    [APP_STATE_OTA]         = PATTERN(100, 100),
    // Все добре: світиться, раз на 3 с коротко гасне («серцебиття»)
    [APP_STATE_OK]          = PATTERN(2900, 100),
    // Немає Wi-Fi: рівне повільне блимання 1 Гц
    [APP_STATE_NO_WIFI]     = PATTERN(500, 500),
    // Немає інтернету / сервер лежить: 2 спалахи, пауза
    [APP_STATE_NO_INTERNET] = PATTERN(150, 250, 150, 1700),
    // Сервер відмовив (ключ, 429): 3 спалахи, пауза
    [APP_STATE_REJECTED]    = PATTERN(150, 250, 150, 250, 150, 1700),
};

static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static esp_timer_handle_t s_timer;
static gpio_num_t s_gpio;
static bool s_active_high;
static const pattern_t *s_pattern;
static size_t s_step;

static void led_write(bool on)
{
    gpio_set_level(s_gpio, on == s_active_high ? 1 : 0);
}

// Виконує поточний крок і заводить таймер на його тривалість.
static void step_cb(void *arg)
{
    taskENTER_CRITICAL(&s_lock);
    const pattern_t *p = s_pattern;
    size_t i = s_step;
    s_step = (i + 1) % p->count;
    taskEXIT_CRITICAL(&s_lock);

    led_write((i % 2) == 0);
    esp_timer_start_once(s_timer, (uint64_t)p->steps[i] * 1000);
}

esp_err_t status_led_init(int gpio, bool active_high)
{
    s_gpio = (gpio_num_t)gpio;
    s_active_high = active_high;
    s_pattern = &PATTERNS[APP_STATE_BOOT];

    gpio_config_t io = {
        .pin_bit_mask = 1ULL << gpio,
        .mode = GPIO_MODE_OUTPUT,
    };
    esp_err_t err = gpio_config(&io);
    if (err != ESP_OK) {
        return err;
    }

    const esp_timer_create_args_t args = {
        .callback = step_cb,
        .name = "status_led",
    };
    err = esp_timer_create(&args, &s_timer);
    if (err != ESP_OK) {
        return err;
    }
    return esp_timer_start_once(s_timer, 1);
}

void status_led_show(app_state_t state)
{
    if (s_timer == NULL || state >= sizeof(PATTERNS) / sizeof(PATTERNS[0])) {
        return;
    }
    ESP_LOGD(TAG, "pattern %s", app_state_name(state));

    // Новий патерн стартує одразу з першого кроку, щоб зміна стану була видна миттєво.
    // Сам крок виконує задача esp_timer, тож GPIO змінюється лише з одного контексту.
    taskENTER_CRITICAL(&s_lock);
    s_pattern = &PATTERNS[state];
    s_step = 0;
    taskEXIT_CRITICAL(&s_lock);
    esp_timer_stop(s_timer);
    esp_timer_start_once(s_timer, 1);
}
