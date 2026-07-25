// components/time_manager/include/time_manager.hpp
#pragma once

#include "interfaces/i_time_manager.hpp"
#include "interfaces/i_hal_sntp.hpp"
#include "interfaces/i_hal_system_time.hpp"

namespace time_manager {

/**
 * @class TimeManager
 * @brief Core implementation of the ITimeManager interface.
 */
class TimeManager : public ITimeManager
{
public:
    TimeManager(idf_hals::IHalSntp& sntp_hal, idf_hals::IHalSystemTime& system_time_hal);
    ~TimeManager() override;

    esp_err_t init(const TimeManagerConfig& config) override;
    esp_err_t start_sntp() override;
    esp_err_t stop_sntp() override;
    esp_err_t request_sync() override;

    bool is_synchronized() const override;
    time_t get_timestamp_sec() const override;
    uint64_t get_timestamp_ms() const override;
    bool get_formatted_time(char* buf, size_t max_len, const char* format = "%Y-%m-%d %H:%M:%S") const override;
    void set_timezone(const char* tz) override;

    TimeSyncPacket create_time_packet() const override;
    esp_err_t sync_from_time_packet(const TimeSyncPacket& packet) override;

private:
    static void sntp_sync_callback(struct timeval *tv);
    void handle_sync_event(struct timeval *tv);

    idf_hals::IHalSntp& sntp_hal_;
    idf_hals::IHalSystemTime& system_time_hal_;
    TimeManagerConfig config_;
    bool is_initialized_{false};
    bool is_sntp_started_{false};
    bool is_synchronized_{false};

    static TimeManager* s_instance;
};

} // namespace time_manager
