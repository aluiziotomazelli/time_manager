# Time Manager Component

[![ESP-IDF Build](https://github.com/aluiziotomazelli/time_manager/actions/workflows/build.yml/badge.svg)](https://github.com/aluiziotomazelli/time_manager/actions/workflows/build.yml)
[![Host Tests](https://github.com/aluiziotomazelli/time_manager/actions/workflows/host_test.yml/badge.svg)](https://github.com/aluiziotomazelli/time_manager/actions/workflows/host_test.yml)
[![Coverage](https://img.shields.io/badge/coverage-report-blue)](https://aluiziotomazelli.github.io/time_manager/index.html)

A lightweight, modular, and dependency-injected C++ library for **ESP-IDF v5.1+** that handles system time, SNTP synchronization, and timezone settings.

## Features

- **Non-blocking Execution**: Uses ESP-IDF's underlying lwIP SNTP service and asynchronous callbacks without spawning dedicated high-priority polling tasks.
- **DHCP Option 42 Support**: Automatically requests and configures NTP servers provided by local DHCP servers, with a fallback server list (e.g., `pool.ntp.org`).
- **Flexible Sync Period**: Sync intervals are configurable (minimum 15s RFC 4330).
- **Node-to-Node Synchronization**: Generates and parses a compact 12-byte packed struct (`TimeSyncPacket`) to synchronize edge nodes via **ESP-NOW** without internet or direct Wi-Fi access.
- **SOLID & SRP Design**: Fully abstract interfaces with constructor injection for easy unit testing and mock frameworks.
- **Host Testing**: Includes a GoogleTest/GoogleMock suite executable on Linux host.

---

## Directory Structure

```text
time_manager/
├── CMakeLists.txt              # ESP-IDF component build system file
├── idf_component.yml           # ESP-IDF component manager manifest
├── README.md                   # Core component documentation
├── API.md                      # Detailed API references
├── CHANGELOG.md                # Version log
├── LICENSE                     # MIT License
├── include/
│   ├── time_manager.hpp        # Concrete TimeManager class
│   ├── time_types.hpp          # Configurations and structs
│   └── interfaces/
│       └── i_time_manager.hpp  # Abstract interface contract
├── src/
│   └── time_manager.cpp        # Implementation logic
└── host_test/
    └── test_time_manager/      # Standalone unit test suite
```

---

## Getting Started

### 1. Register Component
Add the component directory to your project's `CMakeLists.txt` `EXTRA_COMPONENT_DIRS` list.

### 2. Basic Configuration and Sntp Sychronization
```cpp
#include "time_manager.hpp"
#include "hal_sntp.hpp"
#include "hal_system_time.hpp"

// Instantiate HAL components (from idf_hals)
idf_hals::HalSntp sntp_hal;
idf_hals::HalSystemTime system_time_hal;

// Instantiate TimeManager
time_manager::TimeManager tm(sntp_hal, system_time_hal);

void app_main() {
    time_manager::TimeManagerConfig config;
    config.use_dhcp_sntp = true; // request NTP from DHCP
    config.timezone = "<-04>4"; // UTC-4, no DST
    config.default_server = "pool.ntp.org";

    // Initialize TimeManager
    if (tm.init(config) == ESP_OK) {
        // Start background SNTP
        tm.start_sntp();
    }
}
```

### 3. Share Time via ESP-NOW
**On Hub Node:**
```cpp
// Create a sync packet when time is synchronized
if (tm.is_synchronized()) {
    time_manager::TimeSyncPacket packet = tm.create_time_packet();
    // Broadcast packet via ESP-NOW to edge nodes...
}
```

**On Water-Tank / Slave Node:**
```cpp
// Upon receiving the TimeSyncPacket payload
void on_time_packet_received(const time_manager::TimeSyncPacket& packet) {
    tm.sync_from_time_packet(packet);
}
```

### 4. DHCP Option 42 (SNTP) Configuration Note
If you configure `config.use_dhcp_sntp = true` to obtain SNTP servers automatically from the local DHCP server, you **MUST** enable `CONFIG_LWIP_DHCP_GET_NTP_SRV=y` in your project's `sdkconfig` (via `menuconfig` or `sdkconfig.defaults`). 

If this option is disabled in `sdkconfig` while `config.use_dhcp_sntp = true` is set, initialization of the SNTP service will fail (returning `ESP_ERR_INVALID_STATE` / 258). If you do not require DHCP-obtained SNTP servers and only wish to use internet pool servers, simply set `config.use_dhcp_sntp = false` and the component will work perfectly without any sdkconfig changes.
