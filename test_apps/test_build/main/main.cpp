#include "esp_log.h"
#include "time_manager.hpp"
#include "hal_sntp.hpp"
#include "hal_system_time.hpp"

extern "C" void app_main(void)
{
    ESP_LOGI("main", "Time Manager Build Test Starting");

    static idf_hals::HalSntp sntp_hal;
    static idf_hals::HalSystemTime system_time_hal;

    time_manager::TimeManager tm(sntp_hal, system_time_hal);

    time_manager::TimeManagerConfig config;
    config.use_dhcp_sntp = true;
    config.smooth_sync = false;
    config.sync_interval_ms = 3600000;
    config.default_server = "pool.ntp.org";
    config.timezone = "<-04>4";

    esp_err_t err = tm.init(config);
    if (err != ESP_OK) {
        ESP_LOGE("main", "Failed to initialize TimeManager: %d", err);
        return;
    }

    err = tm.start_sntp();
    if (err != ESP_OK) {
        ESP_LOGE("main", "Failed to start SNTP: %d", err);
        return;
    }

    bool synced = tm.is_synchronized();
    ESP_LOGI("main", "Is synchronized: %s", synced ? "yes" : "no");

    char buf[64];
    if (tm.get_formatted_time(buf, sizeof(buf))) {
        ESP_LOGI("main", "Formatted time: %s", buf);
    }

    ESP_LOGI("main", "Time Manager Build Test Passed");
}
