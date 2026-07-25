// components/time_manager/include/interfaces/i_time_manager.hpp
#pragma once

#include "esp_err.h"
#include "time_types.hpp"
#include <time.h>

namespace time_manager {

/**
 * @interface ITimeManager
 * @brief Public interface for application time management.
 */
class ITimeManager
{
public:
    virtual ~ITimeManager() = default;

    /**
     * @brief Initialize TimeManager with configuration.
     * @param config Configuration struct.
     * @return ESP_OK on success, or an error code.
     */
    virtual esp_err_t init(const TimeManagerConfig& config) = 0;

    /**
     * @brief Start the SNTP client.
     * @return ESP_OK on success, or an error code.
     */
    virtual esp_err_t start_sntp() = 0;

    /**
     * @brief Stop the SNTP client.
     * @return ESP_OK on success, or an error code.
     */
    virtual esp_err_t stop_sntp() = 0;

    /**
     * @brief Manually request an immediate SNTP synchronization attempt.
     * @return ESP_OK on success, or an error code.
     */
    virtual esp_err_t request_sync() = 0;

    /**
     * @brief Check if the system clock has been successfully synchronized.
     * @return true if synchronized, false otherwise.
     */
    virtual bool is_synchronized() const = 0;

    /**
     * @brief Get current epoch timestamp in seconds.
     * @return Epoch timestamp.
     */
    virtual time_t get_timestamp_sec() const = 0;

    /**
     * @brief Get current epoch timestamp in milliseconds.
     * @return Epoch timestamp in milliseconds.
     */
    virtual uint64_t get_timestamp_ms() const = 0;

    /**
     * @brief Format local time into buffer.
     * @param buf Destination buffer.
     * @param max_len Size of destination buffer.
     * @param format strftime format specifier.
     * @return true on success, false if buffer is too small.
     */
    virtual bool get_formatted_time(char* buf, size_t max_len, const char* format = "%Y-%m-%d %H:%M:%S") const = 0;

    /**
     * @brief Dynamically set timezone.
     * @param tz POSIX timezone string.
     */
    virtual void set_timezone(const char* tz) = 0;

    /**
     * @brief Create a packet for broadcasting time sync to other nodes.
     * @return TimeSyncPacket.
     */
    virtual TimeSyncPacket create_time_packet() const = 0;

    /**
     * @brief Synchronize system time using a packet received from another node.
     * @param packet TimeSyncPacket.
     * @return ESP_OK on success, or an error code.
     */
    virtual esp_err_t sync_from_time_packet(const TimeSyncPacket& packet) = 0;
};

} // namespace time_manager
