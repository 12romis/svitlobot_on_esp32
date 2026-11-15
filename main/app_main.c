#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mdns.h"

#include "app_state.h"
#include "status_led.h"
#include "wifi_link.h"
#include "pinger.h"
#include "ota_server.h"

static const char *TAG = "svitlobot";

static void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static void init_mdns(void)
{
    if (mdns_init() != ESP_OK) {
        ESP_LOGW(TAG, "mDNS init failed, use IP from router");
        return;
    }
    mdns_hostname_set(CONFIG_SVITLOBOT_HOSTNAME);
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
}

void app_main(void)
{
    // Діод — першим, щоб навіть зависання на старті було видно як «старт».
    ESP_ERROR_CHECK(status_led_init(CONFIG_SVITLOBOT_LED_GPIO, CONFIG_SVITLOBOT_LED_ACTIVE_HIGH));
    app_state_init(status_led_show);

    esp_reset_reason_t reason = esp_reset_reason();
    if (reason == ESP_RST_TASK_WDT || reason == ESP_RST_PANIC || reason == ESP_RST_INT_WDT ||
        reason == ESP_RST_BROWNOUT) {
        ESP_LOGW(TAG, "abnormal reset, reason %d", reason);
    } else {
        ESP_LOGI(TAG, "boot, reset reason %d", reason);
    }

    init_nvs();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    if (CONFIG_SVITLOBOT_WIFI_SSID[0] == '\0' || CONFIG_SVITLOBOT_CHANNEL_KEY[0] == '\0') {
        // Без налаштувань пінгувати нікуди — показуємо «сервер відмовив» (проблема конфігу).
        ESP_LOGE(TAG, "Wi-Fi SSID or channel key not set: idf.py menuconfig -> Svitlobot");
        app_state_set(APP_STATE_REJECTED);
        return;
    }

    const wifi_link_config_t wifi = {
        .ssid = CONFIG_SVITLOBOT_WIFI_SSID,
        .password = CONFIG_SVITLOBOT_WIFI_PASSWORD,
        .hostname = CONFIG_SVITLOBOT_HOSTNAME,
        .on_connected = pinger_wake,
    };
    ESP_ERROR_CHECK(wifi_link_start(&wifi));
    init_mdns();

#if CONFIG_SVITLOBOT_OTA_ENABLE
    ota_server_start(CONFIG_SVITLOBOT_OTA_TOKEN);
#endif

    const pinger_config_t ping = {
        .url = CONFIG_SVITLOBOT_PING_URL,
        .channel_key = CONFIG_SVITLOBOT_CHANNEL_KEY,
        .interval_s = CONFIG_SVITLOBOT_PING_INTERVAL_S,
        .timeout_ms = CONFIG_SVITLOBOT_HTTP_TIMEOUT_MS,
    };
    ESP_ERROR_CHECK(pinger_start(&ping));
}
