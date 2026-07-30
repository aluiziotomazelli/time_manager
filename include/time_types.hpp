// components/time_manager/include/time_types.hpp
#pragma once

#include <cstdint>

namespace time_manager {

/**
 * @brief Source of time synchronization.
 */
enum class TimeSyncSource : uint8_t
{
    UNKNOWN = 0,
    SNTP = 1,
    MANUAL = 2,
    ESP_NOW = 3
};

/**
 * @brief Compact 12-byte packed struct for node-to-node time sync over ESP-NOW.
 */
struct __attribute__((packed)) TimeSyncPacket
{
    uint64_t timestamp_ms;      ///< Epoch timestamp in milliseconds (UTC)
    int16_t tz_offset_min;      ///< Timezone offset in minutes (e.g., -240 for UTC-4)
    TimeSyncSource sync_source; ///< Source of synchronization (TimeSyncSource)
    uint8_t flags;              ///< Bit 0: is_valid (1 if synchronized, 0 otherwise)
};

/**
 * @brief Configuration for the TimeManager.
 */
struct TimeManagerConfig
{
    bool use_dhcp_sntp = true;                   ///< Request NTP server via DHCP (Option 42)
    bool smooth_sync = false;                    ///< Smooth adjtime vs immediate step update
    uint32_t sync_interval_ms = 3600000;         ///< Re-sync interval (default 1 hour = 3600000 ms)
    const char* default_server = "pool.ntp.org"; ///< Primary NTP fallback server
    const char* timezone = "<-04>4";             ///< Generic POSIX TZ string (UTC-4, no DST)
};

} // namespace time_manager
