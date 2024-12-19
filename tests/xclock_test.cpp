#include "xbase.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <thread>

using namespace xsdk;

// NOLINTBEGIN(*)
// We disable assert definition, because these tests should be run only while collecting code coverage or in release
// mode
#ifdef NDEBUG
TEST(xclock_tests, clock_create_with_null_sync_gen)
{
    auto my_clock = xclock::Create(nullptr);
    EXPECT_FALSE(my_clock);
}

TEST(xclock_tests, clock_clone_with_null_input_clock)
{
    auto my_clock = xclock::Clone(nullptr);
    EXPECT_FALSE(my_clock);
}
#endif

TEST(xclock_tests, check_hires_time)
{
    auto hr_time = xclock::HighResTime();
    EXPECT_TRUE(hr_time);
    auto t1 = xclock::HighResTime();
    std::this_thread::sleep_for(std::chrono::nanoseconds(100));
    EXPECT_GT(xclock::HighResTime(), t1) << " time should go forward";
}

TEST(xclock_tests, create_hires_clock)
{
    auto hr_clock = xclock::HighResClock(true);
    EXPECT_TRUE(hr_clock);
    auto t1 = hr_clock->Time();
    EXPECT_GT(hr_clock->Time(t1), 0);
}

TEST(xclock_tests, hires_clock_time_with_no_val)
{
    auto hr_clock = xclock::HighResClock(true);
    EXPECT_TRUE(hr_clock);
    auto t1 = hr_clock->Time(time64::kNoVal);
    EXPECT_GT(hr_clock->SyncGenerator()->Timestamp(), t1);
}

TEST(xclock_tests, create_clock_hr)
{
    auto hr_clock = xclock::ClockHR {time64::kMinute, true};
    auto t1       = hr_clock.Time();
    EXPECT_GT(hr_clock.Time() - t1, 0);
}

TEST(xclock_tests, clock_laps)
{
    auto hr_clock = xclock::Create(xclock::HighResSyncGen(false), xclock::SysTime());
    EXPECT_TRUE(hr_clock);

    auto res = hr_clock->ResetLap(0);
    EXPECT_TRUE(res.first);
    auto t1 = xclock::HighResTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(900));

    auto res2 = hr_clock->ResetLap(time64::kSecond);
    EXPECT_FALSE(res2.first);

    auto t2  = xclock::HighResTime();
    auto lap = hr_clock->Lap();
    EXPECT_GE(lap, t2 - t1);
    EXPECT_GT(time64::kSecond, lap);
}

TEST(xclock_tests, clock_laps_mutithreads)
{
    auto hr_clock = xclock::Create(xclock::HighResSyncGen(false), xclock::SysTime());
    EXPECT_TRUE(hr_clock);

    auto init_reset = hr_clock->ResetLap(0);
    EXPECT_TRUE(init_reset.first);

    std::atomic<bool>        stop        = false;
    size_t                   num_threads = 100;
    std::vector<std::thread> threads;
    std::vector<bool>        reset_ids(num_threads, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(900));

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&, i = i]() {
            while (!stop) {
                auto res = hr_clock->ResetLap(time64::kSecond);
                if (res.first) {
                    reset_ids[i] = true;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    stop = true;
                    break;
                }
            }
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    stop = true;
    std::for_each(threads.begin(), threads.end(), [](auto& th) { th.join(); });

    size_t count_reseters = 0;
    for (int i = 0; i < num_threads; i++) {
        if (reset_ids[i] == true) {
            count_reseters++;
        }
    }

    EXPECT_EQ(count_reseters, 1) << "we can reset lap multiple time from differennt threads: " << count_reseters;
}

TEST(xclock_tests, clock_hr_laps)
{
    auto hr_clock = xclock::ClockHR {};

    auto res = hr_clock.ResetLap(0);
    EXPECT_TRUE(res.first);
    auto t1 = xclock::HighResTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(900));

    auto res2 = hr_clock.ResetLap(time64::kSecond);
    EXPECT_FALSE(res2.first);

    auto t2  = xclock::HighResTime();
    auto lap = hr_clock.Lap();
    EXPECT_GE(lap, t2 - t1);
    EXPECT_GT(time64::kSecond, lap);
}

TEST(xclock_tests, clock_hr_laps_msec)
{
    auto hr_clock = xclock::ClockHR {};

    auto [success1, t1] = hr_clock.ResetLapMsec(0);
    EXPECT_TRUE(success1);
    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    auto [success2, t2] = hr_clock.ResetLapMsec(500);
    EXPECT_FALSE(success2);

    auto lap = hr_clock.LapMsec();
    EXPECT_GE(lap, t2 - t1);
    EXPECT_GT(500, lap);
}

TEST(xclock_tests, clock_hr_reset_time)
{
    auto hr_clock = xclock::ClockHR {};

    hr_clock.ResetTime(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto t1 = hr_clock.Time();
    hr_clock.ResetTime(0);
    auto t2 = hr_clock.Time();
    EXPECT_GT(t1, t2);
}

TEST(xclock_tests, clock_hr_reset_time_msec)
{
    auto hr_clock = xclock::ClockHR {};

    hr_clock.ResetTimeMsec(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto t1 = hr_clock.TimeMsec();
    hr_clock.ResetTimeMsec(100);
    auto t2 = hr_clock.TimeMsec();
    EXPECT_GT(t1, t2);
}

TEST(xclock_tests, sync_gen_std_tests)
{
    auto sys_gen = xclock::SyncGenStd<std::chrono::system_clock>(true);

    EXPECT_TRUE(sys_gen->IsMonotonicIncrease());
    EXPECT_EQ(sys_gen->TicksPerSeconds(), time64::kSecond);
    for (int i = 0; i < 1000; i++) {
        auto ts1 = sys_gen->Timestamp();
        auto ts2 = sys_gen->Timestamp();
        EXPECT_GT(ts2, ts1) << " timestamps are equal, i: " << i;
    }
    EXPECT_GE(sys_gen->Timestamp(1000), sys_gen->Timestamp(1000));

    auto sys_gen2  = xclock::SyncGenStd<std::chrono::system_clock>(false);
    bool has_equal = false;
    for (int i = 0; i < 1000000; i++) {
        if (sys_gen2->Timestamp() == sys_gen2->Timestamp()) {
            has_equal = true;
            break;
        }
    }
    EXPECT_TRUE(has_equal) << "Non monotonic increase sync generator doesn't give same values of timestamps";
}

TEST(xclock_tests, utc_tests)
{
    auto time_utc = xclock::UtcTime();
    auto time_sys = xclock::SysClock(false)->Time(time64::kEpochSysToFileClock);
    std::cout << "utc:" << time_utc << " sys:" << time_sys << std::endl;
    EXPECT_LT(std::abs(time_utc - time_sys), time64::FromMsec(100)) << "Times wrong";
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    auto step_utc = xclock::UtcClock(false)->Time(time_utc);
    auto step_sys = xclock::SysClock(false)->Time(time_sys + time64::kEpochSysToFileClock);
    EXPECT_LT(std::abs(step_utc - step_sys), time64::FromMsec(100)) << "Steps wrong";
    EXPECT_GT(step_utc, time64::FromMsec(1900));
    EXPECT_LT(step_utc, time64::FromMsec(2100));
}

TEST(xclock_tests, utc_performance)
{
    volatile bool stopped = false;

    int64_t     max      = 0;
    int64_t     summ     = 0;
    size_t      same_val = 0;
    size_t      counter1 = 0;
    std::thread utc_th([&]() {
        int64_t t_prev = xclock::UtcTime();
        while (!stopped) {
            auto t = xclock::UtcTime();
            if (t == t_prev)
                ++same_val;
            max = std::max(t - t_prev, max);
            summ += t - t_prev;
            t_prev = t;
            ++counter1;
        }
    });
    int64_t     max2      = 0;
    int64_t     summ2     = 0;
    size_t      same_val2 = 0;
    size_t      counter2  = 0;
    std::thread sys_th([&]() {
        int64_t t_prev = xclock::SysTime();
        while (!stopped) {
            auto t = xclock::SysTime();
            if (t == t_prev)
                ++same_val2;

            max2 = std::max(t - t_prev, max2);
            summ2 += t - t_prev;
            t_prev = t;
            ++counter2;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    stopped = true;
    utc_th.join();
    sys_th.join();

    std::cout << "UtcSysTime:" << counter1 << " eq:" << same_val << "(" << counter1 - same_val << ")"
              << " gap:" << max << std::endl;
    std::cout << "SysTime(std):" << counter2 << " eq:" << same_val2 << "(" << counter2 - same_val2 << ")"
              << " gap:" << max << std::endl;

    // EXPECT_FALSE(1);
}

TEST(xclock_tests, create_clone_clock)
{
    auto clock_p = xclock::Create(xclock::SteadySyncGen(true), 0);
    auto t       = clock_p->Time();
    EXPECT_GE(t, 0);
    EXPECT_LE(t, time64::FromMsec(10));

    auto clone_p = xclock::Clone(clock_p.get());
    ASSERT_TRUE(clone_p);
    t = clone_p->Time();

    EXPECT_GE(t, 0);
    EXPECT_LE(t, time64::FromMsec(10));

    auto clone_p2 = xclock::Clone(clone_p.get(), xclock::AdjustType::kClockOffset, time64::FromSec(10));

    t       = clone_p->Time();
    auto t2 = clone_p2->Time();

    EXPECT_LE(std::abs((t2 - t) - time64::FromSec(10)), time64::FromMsec(1));

    auto clone_p3 = xclock::Clone(clone_p.get(), xclock::AdjustType::kStartTime, time64::kDay);
    t             = clone_p3->Time();

    EXPECT_GE(t, time64::kDay);
    EXPECT_LE(t, time64::kDay + time64::FromMsec(10));
}

// NOLINTEND(*)