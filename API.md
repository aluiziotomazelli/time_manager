# Time Manager Component API Reference

Detailed programming interface reference for the `time_manager` component.

---

## Data Types

### `TimeSyncSource`
```cpp
enum class TimeSyncSource : uint8_t {
    UNKNOWN = 0,
    SNTP = 1,
    MANUAL = 2,
    ESP_NOW = 3
};
```
Defines the source that updated the clock.

### `TimeSyncPacket`
```cpp
struct __attribute__((packed)) TimeSyncPacket {
    uint64_t timestamp_ms;   ///< Epoch timestamp in milliseconds (UTC)
    int16_t  tz_offset_min;  ///< Timezone offset in minutes (e.g., -240 for UTC-4)
    uint8_t  sync_source;    ///< Source of synchronization (TimeSyncSource)
    uint8_t  flags;          ///< Bit 0: is_valid (1 if synchronized, 0 otherwise)
};
```
A 12-byte packed struct used for node-to-node time distribution over ESP-NOW.

### `TimeManagerConfig`
```cpp
struct TimeManagerConfig {
    bool use_dhcp_sntp = true;            ///< Request NTP server via DHCP (Option 42)
    bool smooth_sync = false;             ///< Smooth adjtime vs immediate step update
    uint32_t sync_interval_ms = 3600000;  ///< Re-sync interval (default 1 hour)
    const char* default_server = "pool.ntp.org"; ///< Primary NTP fallback server
    const char* timezone = "<-04>4";      ///< Generic POSIX TZ string (UTC-4, no DST)
};
```
Configuration settings passed to the `init()` function.

---

## Class Reference

### `ITimeManager` (Abstract Interface)
Defined in `time_manager/include/interfaces/i_time_manager.hpp`.

#### `virtual esp_err_t init(const TimeManagerConfig& config) = 0`
Initializes the manager, applies the timezone offset, and sets the internal state.
- **Parameters**: `config` - TimeManagerConfig struct.
- **Returns**: `ESP_OK` on success, `ESP_ERR_INVALID_STATE` if already initialized.

#### `virtual esp_err_t start_sntp() = 0`
Initializes and starts the underlying ESP-IDF SNTP client.
- **Returns**: `ESP_OK` on success, or ESP-IDF error code.

#### `virtual esp_err_t stop_sntp() = 0`
Deinitializes and stops the SNTP client.
- **Returns**: `ESP_OK` on success.

#### `virtual esp_err_t request_sync() = 0`
Requests an immediate manual synchronization attempt from the configured NTP server.
- **Returns**: `ESP_OK` on success, `ESP_FAIL` on failure.

#### `virtual bool is_synchronized() const = 0`
Checks if the system clock has a valid synchronization state.
- **Returns**: `true` if system clock has synchronized (callback fired or year > 2020), `false` otherwise.

#### `virtual time_t get_timestamp_sec() const = 0`
Gets current Unix epoch timestamp in seconds.
- **Returns**: `time_t` timestamp.

#### `virtual uint64_t get_timestamp_ms() const = 0`
Gets current Unix epoch timestamp in milliseconds.
- **Returns**: `uint64_t` timestamp in ms.

#### `virtual bool get_formatted_time(char* buf, size_t max_len, const char* format = "%Y-%m-%d %H:%M:%S") const = 0`
Formats local system time into a text buffer using standard `strftime` specifiers.
- **Parameters**: 
  - `buf` - Destination char array.
  - `max_len` - Size of destination array.
  - `format` - Formatting string (default: `"YYYY-MM-DD HH:MM:SS"`).
- **Returns**: `true` on success, `false` if `buf` is null or `max_len` is 0.

#### `virtual void set_timezone(const char* tz) = 0`
Configures local time zone offset using POSIX timezone string syntax.
- **Parameters**: `tz` - POSIX timezone string (e.g. `"<-04>4"`).

#### `virtual TimeSyncPacket create_time_packet() const = 0`
Creates a `TimeSyncPacket` payload populated with the current clock state and timezone offset.
- **Returns**: `TimeSyncPacket` struct.

#### `virtual esp_err_t sync_from_time_packet(const TimeSyncPacket& packet) = 0`
Synchronizes the local system clock and timezone settings using a received `TimeSyncPacket`.
- **Parameters**: `packet` - TimeSyncPacket struct.
- **Returns**: `ESP_OK` on success, or ESP-IDF error code.
