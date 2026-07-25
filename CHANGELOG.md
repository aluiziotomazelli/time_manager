# Changelog

All notable changes to the `time_manager` component will be documented in this file.

## [0.1.0] - 2026-07-25

### Added
- Initial implementation of the standalone C++ `time_manager` component.
- Dependency injection pattern wrapping ESP-IDF SNTP and POSIX system time via `idf_hals`.
- Support for requesting NTP servers dynamically via DHCP (Option 42) with fallback to custom server list.
- Configurable sync modes (smooth vs immediate step adjustment) and periodic synchronization intervals.
- Support for serializing and deserializing a compact 12-byte packed struct (`TimeSyncPacket`) for node-to-node time distribution over ESP-NOW.
- Comprehensive GoogleTest suite executing on Linux host.
- Independent GitHub Actions build and host test CI workflows.
