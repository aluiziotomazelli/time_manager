// components/time_manager/src/time_manager.cpp
#include "time_manager.hpp"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"

#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

static const char* TAG = "TimeManager";

namespace time_manager {

TimeManager* TimeManager::s_instance = nullptr;

TimeManager::TimeManager(idf_hals::IHalSntp& sntp_hal, idf_hals::IHalSystemTime& system_time_hal)
    : sntp_hal_(sntp_hal)
    , system_time_hal_(system_time_hal)
{
}

TimeManager::~TimeManager()
{
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

esp_err_t TimeManager::init(const TimeManagerConfig& config)
{
    if (is_initialized_) {
        ESP_LOGE(TAG, "TimeManager already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    config_ = config;
    s_instance = this;

    // Set standard timezone offset
    set_timezone(config_.timezone);

    is_initialized_ = true;
    ESP_LOGI(TAG, "TimeManager initialized with TZ=%s", config_.timezone);
    return ESP_OK;
}

esp_err_t TimeManager::start_sntp()
{
    if (!is_initialized_) {
        ESP_LOGE(TAG, "TimeManager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    if (is_sntp_started_) {
        return ESP_OK;
    }

    idf_hals::HalSntpConfig sntp_config = {};
    sntp_config.smooth_sync = config_.smooth_sync;
    sntp_config.server_from_dhcp = config_.use_dhcp_sntp;
    sntp_config.default_server = config_.default_server;
    sntp_config.sync_interval_ms = config_.sync_interval_ms;
    sntp_config.sync_cb = &TimeManager::sntp_sync_callback;

    esp_err_t err = sntp_hal_.init(sntp_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SNTP HAL: %d", err);
        return err;
    }

    err = sntp_hal_.start();
    if (err == ESP_OK) {
        is_sntp_started_ = true;
        if (config_.sync_interval_ms >= 15000) {
            sntp_hal_.set_sync_interval(config_.sync_interval_ms);
        }
        ESP_LOGI(TAG, "SNTP started successfully");
    }
    else {
        ESP_LOGE(TAG, "Failed to start SNTP: %d", err);
    }
    return err;
}

esp_err_t TimeManager::stop_sntp()
{
    if (!is_sntp_started_) {
        return ESP_OK;
    }
    sntp_hal_.deinit();
    is_sntp_started_ = false;
    ESP_LOGI(TAG, "SNTP stopped");
    return ESP_OK;
}

esp_err_t TimeManager::request_sync()
{
    if (!is_sntp_started_) {
        ESP_LOGE(TAG, "Cannot request sync, SNTP not started");
        return ESP_ERR_INVALID_STATE;
    }
    bool success = sntp_hal_.restart();
    if (success) {
        ESP_LOGI(TAG, "SNTP synchronization requested");
        return ESP_OK;
    }
    return ESP_FAIL;
}

bool TimeManager::is_synchronized() const
{
    if (is_synchronized_) {
        return true;
    }
    // Fallback: check if the system clock has a valid synchronized year (e.g. > Jan 1 2020)
    time_t now = 0;
    system_time_hal_.time(&now);
    struct tm timeinfo;
    system_time_hal_.localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year > 120) { // year since 1900, 120 is 2020
        return true;
    }
    return false;
}

time_t TimeManager::get_timestamp_sec() const
{
    time_t now = 0;
    system_time_hal_.time(&now);
    return now;
}

uint64_t TimeManager::get_timestamp_ms() const
{
    struct timeval tv;
    system_time_hal_.gettimeofday(&tv, nullptr);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

bool TimeManager::get_formatted_time(char* buf, size_t max_len, const char* format) const
{
    if (!buf || max_len == 0 || !format) {
        return false;
    }
    time_t now = get_timestamp_sec();
    struct tm timeinfo;
    system_time_hal_.localtime_r(&now, &timeinfo);
    size_t written = strftime(buf, max_len, format, &timeinfo);
    return written > 0;
}

void TimeManager::set_timezone(const char* tz)
{
    if (!tz) {
        return;
    }
    system_time_hal_.setenv("TZ", tz, 1);
    system_time_hal_.tzset();
    ESP_LOGD(TAG, "Timezone set to %s", tz);
}

// Helper to convert struct tm (interpreted as UTC) to epoch seconds
static time_t tm_to_epoch(const struct tm& tm)
{
    int year = tm.tm_year + 1900;
    int month = tm.tm_mon + 1; // 1-12
    int day = tm.tm_mday;

    if (month < 3) {
        month += 12;
        year -= 1;
    }
    long long days = day + (153 * month - 457) / 5 + 365 * year + year / 4 - year / 100 + year / 400 - 719469;
    return days * 86400 + tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec;
}

TimeSyncPacket TimeManager::create_time_packet() const
{
    TimeSyncPacket packet = {};
    packet.timestamp_ms = get_timestamp_ms();

    time_t now = get_timestamp_sec();
    struct tm local_tm = {};
    system_time_hal_.localtime_r(&now, &local_tm);

    // Calculate difference between local time representation and UTC time
    time_t local_sec = tm_to_epoch(local_tm);
    long diff_sec = (long)(local_sec - now);

    packet.tz_offset_min = (int16_t)(diff_sec / 60);
    packet.sync_source = TimeSyncSource::SNTP;
    packet.flags = is_synchronized() ? 0x01 : 0x00;
    return packet;
}

esp_err_t TimeManager::sync_from_time_packet(const TimeSyncPacket& packet)
{
    if (!(packet.flags & 0x01)) {
        ESP_LOGE(TAG, "Cannot sync from packet: packet is not valid");
        return ESP_ERR_INVALID_ARG;
    }

    struct timeval tv;
    tv.tv_sec = (time_t)(packet.timestamp_ms / 1000);
    tv.tv_usec = (suseconds_t)((packet.timestamp_ms % 1000) * 1000);

    int ret = system_time_hal_.settimeofday(&tv, nullptr);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to set system time: %d", ret);
        return ESP_FAIL;
    }

    // Set timezone from offset.
    // e.g. -240 minutes = UTC-4.
    // POSIX TZ format std offset: offset is positive for west of UTC.
    char tz_str[32];
    int16_t offset_hours = -packet.tz_offset_min / 60;
    if (offset_hours >= 0) {
        snprintf(tz_str, sizeof(tz_str), "UTC%d", offset_hours);
    }
    else {
        snprintf(tz_str, sizeof(tz_str), "UTC%d", offset_hours);
    }
    set_timezone(tz_str);

    is_synchronized_ = true;
    ESP_LOGI(
        TAG,
        "System time synchronized from node packet. Epoch: %llu ms, TZ offset: %d min",
        packet.timestamp_ms,
        packet.tz_offset_min);
    return ESP_OK;
}

void TimeManager::sntp_sync_callback(struct timeval* tv)
{
    if (s_instance) {
        s_instance->handle_sync_event(tv);
    }
}

void TimeManager::handle_sync_event(struct timeval* tv)
{
    is_synchronized_ = true;
    ESP_LOGI(TAG, "SNTP synchronized successfully. Current time: %ld sec", tv->tv_sec);
}

} // namespace time_manager
