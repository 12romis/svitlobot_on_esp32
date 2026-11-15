#include "pinger.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_ota_ops.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "app_state.h"
#include "wifi_link.h"

static const char *TAG = "pinger";

#define TASK_STACK      8192   // TLS-рукостискання потребує запасу
#define TASK_PRIO       5
#define WDT_FEED_MS     1000

static pinger_config_t s_cfg;
static char s_url[384];
static TaskHandle_t s_task;

// Ключ підставляється в query string, тож кодуємо все, крім безпечних символів.
static void url_encode(char *dst, size_t dst_len, const char *src)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t o = 0;
    for (; *src && o + 4 < dst_len; src++) {
        unsigned char c = (unsigned char)*src;
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            dst[o++] = c;
        } else {
            dst[o++] = '%';
            dst[o++] = hex[c >> 4];
            dst[o++] = hex[c & 0x0F];
        }
    }
    dst[o] = '\0';
}

static app_state_t classify(esp_err_t err, int status)
{
    if (err != ESP_OK || status >= 500) {
        return APP_STATE_NO_INTERNET;
    }
    if (status >= 200 && status < 300) {
        return APP_STATE_OK;
    }
    return APP_STATE_REJECTED; // 4xx (400 — неправильний ключ, 429 — забагато запитів) та інше неочікуване
}

static app_state_t ping_once(void)
{
    esp_http_client_config_t hc = {
        .url = s_url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = (int)s_cfg.timeout_ms,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .disable_auto_redirect = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&hc);
    if (client == NULL) {
        return APP_STATE_NO_INTERNET;
    }

    int64_t t0 = esp_timer_get_time();
    int status = 0;
    char body[128] = "";
    esp_err_t err = esp_http_client_open(client, 0);
    if (err == ESP_OK) {
        if (esp_http_client_fetch_headers(client) < 0) {
            err = ESP_FAIL;
        } else {
            status = esp_http_client_get_status_code(client);
            int n = esp_http_client_read_response(client, body, sizeof(body) - 1);
            body[n > 0 ? n : 0] = '\0';
        }
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    app_state_t result = classify(err, status);
    ESP_LOGI(TAG, "GET -> %s, status %d, %lld ms, rssi %d, body \"%s\"",
             esp_err_to_name(err), status, (esp_timer_get_time() - t0) / 1000, wifi_link_rssi(), body);
    return result;
}

// Нова прошивка після OTA вважається робочою, коли пройшла повний цикл із Wi-Fi без падінь.
// Сам результат пінгу не важливий: інтернету може не бути з причин, не повʼязаних із прошивкою.
static void confirm_firmware_once(void)
{
    static bool done;
    if (done) {
        return;
    }
    done = true;

    esp_ota_img_states_t st;
    if (esp_ota_get_state_partition(esp_ota_get_running_partition(), &st) == ESP_OK &&
        st == ESP_OTA_IMG_PENDING_VERIFY) {
        ESP_LOGI(TAG, "new firmware confirmed, rollback cancelled");
        esp_ota_mark_app_valid_cancel_rollback();
    }
}

// Чекає до наступного пінгу, годуючи watchdog. Перерване pinger_wake().
static void wait_next(TickType_t cycle_start)
{
    const TickType_t period = pdMS_TO_TICKS(s_cfg.interval_s * 1000);
    while (xTaskGetTickCount() - cycle_start < period) {
        esp_task_wdt_reset();
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(WDT_FEED_MS)) > 0) {
            ESP_LOGI(TAG, "woken up early");
            return;
        }
    }
}

static void pinger_task(void *arg)
{
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    for (;;) {
        TickType_t cycle_start = xTaskGetTickCount();
        esp_task_wdt_reset();

        // Без Wi-Fi стан уже виставив wifi_link; після підключення нас розбудять.
        if (wifi_link_is_connected()) {
            app_state_set(ping_once());
            confirm_firmware_once();
        }
        wait_next(cycle_start);
    }
}

esp_err_t pinger_start(const pinger_config_t *cfg)
{
    s_cfg = *cfg;

    char key[160];
    url_encode(key, sizeof(key), cfg->channel_key);
    int n = snprintf(s_url, sizeof(s_url), "%s%schannel_key=%s",
                     cfg->url, strchr(cfg->url, '?') ? "&" : "?", key);
    if (n < 0 || n >= (int)sizeof(s_url)) {
        return ESP_ERR_INVALID_SIZE;
    }

    if (xTaskCreate(pinger_task, "pinger", TASK_STACK, NULL, TASK_PRIO, &s_task) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void pinger_wake(void)
{
    if (s_task) {
        xTaskNotifyGive(s_task);
    }
}
