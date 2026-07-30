// components/time_manager/host_test/test_time_manager/main/test_time_manager.cpp
#include "time_manager.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock_hal_sntp.hpp"
#include "mock_hal_system_time.hpp"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArgPointee;

namespace time_manager {

class TimeManagerTest : public ::testing::Test
{
protected:
    void SetUp() override {}

    idf_hals::MockHalSntp mock_sntp_;
    idf_hals::MockHalSystemTime mock_system_time_;
};

TEST_F(TimeManagerTest, TestInitAppliesTimezone)
{
    TimeManager tm(mock_sntp_, mock_system_time_);

    TimeManagerConfig config;
    config.timezone = "<-04>4";

    EXPECT_CALL(mock_system_time_, setenv(testing::StrEq("TZ"), testing::StrEq("<-04>4"), 1)).WillOnce(Return(0));
    EXPECT_CALL(mock_system_time_, tzset()).Times(1);

    EXPECT_EQ(tm.init(config), ESP_OK);

    // Double init should fail
    EXPECT_EQ(tm.init(config), ESP_ERR_INVALID_STATE);
}

TEST_F(TimeManagerTest, TestStartSntpSuccess)
{
    TimeManager tm(mock_sntp_, mock_system_time_);

    TimeManagerConfig config;
    config.use_dhcp_sntp = true;
    config.smooth_sync = false;
    config.sync_interval_ms = 3600000;
    config.default_server = "pool.ntp.org";
    config.timezone = "<-04>4";

    EXPECT_CALL(mock_system_time_, setenv(_, _, _)).WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_time_, tzset()).WillRepeatedly(Return());

    ASSERT_EQ(tm.init(config), ESP_OK);

    idf_hals::HalSntpConfig captured_config = {};
    EXPECT_CALL(mock_sntp_, init(_)).WillOnce(DoAll(SaveArg<0>(&captured_config), Return(ESP_OK)));

    EXPECT_CALL(mock_sntp_, start()).WillOnce(Return(ESP_OK));

    EXPECT_CALL(mock_sntp_, set_sync_interval(3600000)).Times(1);

    EXPECT_EQ(tm.start_sntp(), ESP_OK);

    // Config assertions
    EXPECT_EQ(captured_config.smooth_sync, false);
    EXPECT_EQ(captured_config.server_from_dhcp, true);
    EXPECT_STREQ(captured_config.default_server, "pool.ntp.org");
    EXPECT_NE(captured_config.sync_cb, nullptr);
}

TEST_F(TimeManagerTest, TestStopSntp)
{
    TimeManager tm(mock_sntp_, mock_system_time_);
    TimeManagerConfig config;

    EXPECT_CALL(mock_system_time_, setenv(_, _, _)).WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_time_, tzset()).WillRepeatedly(Return());

    ASSERT_EQ(tm.init(config), ESP_OK);

    EXPECT_CALL(mock_sntp_, init(_)).WillOnce(Return(ESP_OK));
    EXPECT_CALL(mock_sntp_, start()).WillOnce(Return(ESP_OK));
    EXPECT_CALL(mock_sntp_, set_sync_interval(_)).Times(1);

    ASSERT_EQ(tm.start_sntp(), ESP_OK);

    // Stop should deinit
    EXPECT_CALL(mock_sntp_, deinit()).Times(1);
    EXPECT_EQ(tm.stop_sntp(), ESP_OK);
}

TEST_F(TimeManagerTest, TestRequestSync)
{
    TimeManager tm(mock_sntp_, mock_system_time_);
    TimeManagerConfig config;

    EXPECT_CALL(mock_system_time_, setenv(_, _, _)).WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_time_, tzset()).WillRepeatedly(Return());

    ASSERT_EQ(tm.init(config), ESP_OK);

    EXPECT_CALL(mock_sntp_, init(_)).WillOnce(Return(ESP_OK));
    EXPECT_CALL(mock_sntp_, start()).WillOnce(Return(ESP_OK));
    EXPECT_CALL(mock_sntp_, set_sync_interval(_)).Times(1);

    ASSERT_EQ(tm.start_sntp(), ESP_OK);

    EXPECT_CALL(mock_sntp_, restart()).WillOnce(Return(true));
    EXPECT_EQ(tm.request_sync(), ESP_OK);
}

TEST_F(TimeManagerTest, TestIsSynchronizedFallback)
{
    TimeManager tm(mock_sntp_, mock_system_time_);
    TimeManagerConfig config;

    EXPECT_CALL(mock_system_time_, setenv(_, _, _)).WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_time_, tzset()).WillRepeatedly(Return());

    ASSERT_EQ(tm.init(config), ESP_OK);

    // Initial state: not synchronized
    EXPECT_CALL(mock_system_time_, time(_)).WillOnce(DoAll(SetArgPointee<0>(1000), Return(1000)));
    struct tm mock_tm = {};
    mock_tm.tm_year = 70; // 1970
    EXPECT_CALL(mock_system_time_, localtime_r(_, _)).WillOnce(DoAll(SetArgPointee<1>(mock_tm), Return(&mock_tm)));

    EXPECT_FALSE(tm.is_synchronized());

    // State with synchronized clock (> 2020)
    EXPECT_CALL(mock_system_time_, time(_)).WillOnce(DoAll(SetArgPointee<0>(1700000000), Return(1700000000)));
    mock_tm.tm_year = 123; // 2023
    EXPECT_CALL(mock_system_time_, localtime_r(_, _)).WillOnce(DoAll(SetArgPointee<1>(mock_tm), Return(&mock_tm)));

    EXPECT_TRUE(tm.is_synchronized());
}

TEST_F(TimeManagerTest, TestGetTimestamp)
{
    TimeManager tm(mock_sntp_, mock_system_time_);
    TimeManagerConfig config;

    EXPECT_CALL(mock_system_time_, setenv(_, _, _)).WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_time_, tzset()).WillRepeatedly(Return());

    ASSERT_EQ(tm.init(config), ESP_OK);

    EXPECT_CALL(mock_system_time_, time(_)).WillOnce(DoAll(SetArgPointee<0>(123456), Return(123456)));
    EXPECT_EQ(tm.get_timestamp_sec(), 123456);

    struct timeval tv = {123456, 789000}; // 123456.789 sec
    EXPECT_CALL(mock_system_time_, gettimeofday(_, nullptr)).WillOnce(DoAll(SetArgPointee<0>(tv), Return(0)));
    EXPECT_EQ(tm.get_timestamp_ms(), 123456789ULL);
}

TEST_F(TimeManagerTest, TestFormattedTime)
{
    TimeManager tm(mock_sntp_, mock_system_time_);
    TimeManagerConfig config;

    EXPECT_CALL(mock_system_time_, setenv(_, _, _)).WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_time_, tzset()).WillRepeatedly(Return());

    ASSERT_EQ(tm.init(config), ESP_OK);

    EXPECT_CALL(mock_system_time_, time(_)).WillOnce(DoAll(SetArgPointee<0>(1700000000), Return(1700000000)));
    struct tm mock_tm = {};
    mock_tm.tm_year = 123;
    mock_tm.tm_mon = 10; // November
    mock_tm.tm_mday = 14;
    mock_tm.tm_hour = 15;
    mock_tm.tm_min = 30;
    mock_tm.tm_sec = 0;
    EXPECT_CALL(mock_system_time_, localtime_r(_, _)).WillOnce(DoAll(SetArgPointee<1>(mock_tm), Return(&mock_tm)));

    char buf[64];
    EXPECT_TRUE(tm.get_formatted_time(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S"));
    EXPECT_STREQ(buf, "2023-11-14 15:30:00");
}

TEST_F(TimeManagerTest, TestCreateTimePacket)
{
    TimeManager tm(mock_sntp_, mock_system_time_);
    TimeManagerConfig config;

    EXPECT_CALL(mock_system_time_, setenv(_, _, _)).WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_time_, tzset()).WillRepeatedly(Return());

    ASSERT_EQ(tm.init(config), ESP_OK);

    struct timeval tv = {1700000000, 500000};
    EXPECT_CALL(mock_system_time_, gettimeofday(_, nullptr)).WillRepeatedly(DoAll(SetArgPointee<0>(tv), Return(0)));
    EXPECT_CALL(mock_system_time_, time(_)).WillRepeatedly(DoAll(SetArgPointee<0>(1700000000), Return(1700000000)));

    struct tm mock_tm = {};
    mock_tm.tm_year = 123; // 2023
    mock_tm.tm_mon = 10;
    mock_tm.tm_mday = 14;
    mock_tm.tm_hour = 18; // 18:13:20 local (represents UTC-4 for 22:13:20 UTC)
    mock_tm.tm_min = 13;
    mock_tm.tm_sec = 20;
    // local time is UTC-4: so 1700000000 corresponds to 22:13:20 UTC.
    // local tm is 18:13:20. Difference is -4 hours = -240 minutes.
    EXPECT_CALL(mock_system_time_, localtime_r(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(mock_tm), Return(&mock_tm)));

    TimeSyncPacket packet = tm.create_time_packet();
    EXPECT_EQ(packet.timestamp_ms, 1700000000500ULL);
    EXPECT_EQ(packet.tz_offset_min, -240);
    EXPECT_EQ(packet.sync_source, TimeSyncSource::SNTP);
    EXPECT_EQ(packet.flags, 0x01); // fallback is_synchronized because tm_year > 120
}

TEST_F(TimeManagerTest, TestSyncFromTimePacket)
{
    TimeManager tm(mock_sntp_, mock_system_time_);
    TimeManagerConfig config;

    EXPECT_CALL(mock_system_time_, setenv(_, _, _)).WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_time_, tzset()).WillRepeatedly(Return());

    ASSERT_EQ(tm.init(config), ESP_OK);

    TimeSyncPacket packet = {};
    packet.timestamp_ms = 1700000000500ULL;
    packet.tz_offset_min = -240; // UTC-4
    packet.flags = 0x01;         // synchronized/valid

    struct timeval expected_tv = {1700000000, 500000};
    EXPECT_CALL(
        mock_system_time_,
        settimeofday(
            testing::Pointer(
                testing::AllOf(
                    testing::Field(&timeval::tv_sec, expected_tv.tv_sec),
                    testing::Field(&timeval::tv_usec, expected_tv.tv_usec))),
            nullptr))
        .WillOnce(Return(0));

    EXPECT_CALL(mock_system_time_, setenv(testing::StrEq("TZ"), testing::StrEq("UTC4"), 1)).WillOnce(Return(0));
    EXPECT_CALL(mock_system_time_, tzset()).Times(1);

    EXPECT_EQ(tm.sync_from_time_packet(packet), ESP_OK);
    EXPECT_TRUE(tm.is_synchronized());
}

} // namespace time_manager
