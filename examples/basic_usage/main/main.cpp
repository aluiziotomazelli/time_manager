// examples/basic_usage/main/main.cpp
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

#include "time_manager.hpp"
#include "hal_sntp.hpp"
#include "hal_system_time.hpp"
#include "example_secrets.h"

static const char *TAG = "main";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Connecting to AP...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected from AP, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {};
    std::strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    std::strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting Time Manager basic usage example");

    // Initialize NVS (required for Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Wi-Fi
    wifi_init_sta();

    // Create HAL dependencies for TimeManager
    static idf_hals::HalSntp sntp_hal;
    static idf_hals::HalSystemTime system_time_hal;

    // Instantiate TimeManager
    time_manager::TimeManager tm(sntp_hal, system_time_hal);

    // Configure (Brazil region UTC-4 without DST)
    time_manager::TimeManagerConfig config;
    config.use_dhcp_sntp = true;
    config.smooth_sync = false;
    config.sync_interval_ms = 3600000; // 1 hour
    config.default_server = "pool.ntp.org";
    config.timezone = "<-04>4"; // UTC-4 timezone

    // Initialize TimeManager
    ret = tm.init(config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TimeManager: %d", ret);
        return;
    }

    // Start SNTP client
    ret = tm.start_sntp();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start SNTP client: %d", ret);
        return;
    }

    ESP_LOGI(TAG, "Waiting for system time to synchronize...");
    int timeout_sec = 30;
    while (!tm.is_synchronized() && timeout_sec > 0) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        timeout_sec--;
    }

    if (tm.is_synchronized()) {
        ESP_LOGI(TAG, "System time successfully synchronized!");
    } else {
        ESP_LOGW(TAG, "System time synchronization timed out. Using fallback clock.");
    }

    // Periodically display current epoch and local formatted time
    for (int i = 0; i < 5; ++i) {
        char time_buf[64];
        if (tm.get_formatted_time(time_buf, sizeof(time_buf))) {
            ESP_LOGI(TAG, "Current Time: %s (Epoch: %lld ms)", time_buf, (long long)tm.get_timestamp_ms());
        } else {
            ESP_LOGE(TAG, "Failed to format time");
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    ESP_LOGI(TAG, "Example execution finished.");
}
