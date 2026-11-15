#include "ota_server.h"

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_log.h"
#include "app_state.h"

static const char *TAG = "ota_server";

static char s_token[96];

// Порівняння за сталий час, щоб токен не можна було підбирати за таймінгом.
static bool token_equal(const char *a, const char *b)
{
    size_t la = strlen(a), lb = strlen(b);
    unsigned char diff = la != lb;
    for (size_t i = 0; i < la && i < lb; i++) {
        diff |= (unsigned char)(a[i] ^ b[i]);
    }
    return diff == 0;
}

static esp_err_t status_get(httpd_req_t *req)
{
    char buf[256];
    const esp_app_desc_t *app = esp_app_get_description();
    snprintf(buf, sizeof(buf),
             "{\"state\":\"%s\",\"version\":\"%s\",\"idf\":\"%s\",\"uptime_s\":%lld,\"reset_reason\":%d}\n",
             app_state_name(app_state_get()), app->version, app->idf_ver,
             esp_timer_get_time() / 1000000, (int)esp_reset_reason());
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, buf);
}

static esp_err_t ota_post(httpd_req_t *req)
{
    char token[sizeof(s_token)] = "";
    if (httpd_req_get_hdr_value_str(req, "X-OTA-Token", token, sizeof(token)) != ESP_OK ||
        !token_equal(token, s_token)) {
        ESP_LOGW(TAG, "rejected: bad token");
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "bad token");
    }

    const esp_partition_t *part = esp_ota_get_next_update_partition(NULL);
    if (part == NULL || req->content_len == 0 || req->content_len > part->size) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad image size");
    }

    esp_ota_handle_t ota;
    esp_err_t err = esp_ota_begin(part, OTA_WITH_SEQUENTIAL_WRITES, &ota);
    if (err != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, esp_err_to_name(err));
    }
    app_state_set_ota(true);
    ESP_LOGI(TAG, "writing %u bytes to %s", (unsigned)req->content_len, part->label);

    char buf[1024];
    size_t left = req->content_len;
    int timeouts = 0;
    while (left > 0 && err == ESP_OK) {
        int n = httpd_req_recv(req, buf, left < sizeof(buf) ? left : sizeof(buf));
        if (n == HTTPD_SOCK_ERR_TIMEOUT && ++timeouts < 3) {
            continue;
        }
        if (n <= 0) {
            err = ESP_FAIL;
            break;
        }
        err = esp_ota_write(ota, buf, n);
        left -= n;
    }
    if (err == ESP_OK) {
        err = esp_ota_end(ota); // перевіряє образ
    } else {
        esp_ota_abort(ota);
    }
    if (err == ESP_OK) {
        err = esp_ota_set_boot_partition(part);
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(err));
        app_state_set_ota(false);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "OTA ok, rebooting");
    httpd_resp_sendstr(req, "OK, rebooting\n");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    return ESP_OK;
}

esp_err_t ota_server_start(const char *token)
{
    if (token == NULL || strlen(token) < 16 || strlen(token) >= sizeof(s_token)) {
        ESP_LOGW(TAG, "OTA token must be 16..%u chars, server not started", (unsigned)sizeof(s_token) - 1);
        return ESP_ERR_INVALID_ARG;
    }
    strlcpy(s_token, token, sizeof(s_token));

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.stack_size = 6144;
    httpd_handle_t server;
    esp_err_t err = httpd_start(&server, &cfg);
    if (err != ESP_OK) {
        return err;
    }

    const httpd_uri_t status = { .uri = "/status", .method = HTTP_GET, .handler = status_get };
    const httpd_uri_t ota = { .uri = "/ota", .method = HTTP_POST, .handler = ota_post };
    httpd_register_uri_handler(server, &status);
    httpd_register_uri_handler(server, &ota);
    return ESP_OK;
}
