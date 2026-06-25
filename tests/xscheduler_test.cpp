#include "xbase.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <future>
#include <thread>

using namespace xsdk;

// We disable assert definition, because these tests should be run only while collecting code coverage or in release
// mode
#ifdef NDEBUG
TEST(xscheduler_test, non_exists_scheduler)
{
    const std::string expected_res = "check_string";
    xbase::ClockHR    clock;
    auto [task_uid, task_future] = xscheduler::ScheduleTask<std::pair<std::string, double>>(nullptr, 5, [&]() {
        return std::make_pair(expected_res, clock.TimeMsec());
    });

    EXPECT_EQ(xbase::kInvalidUid, task_uid) << "Task uid should be invalid: " << task_uid;
    EXPECT_FALSE(task_future.valid());
}

TEST(xscheduler_test, non_exists_pf)
{
    auto scheduler_p   = xscheduler::CreateScheduler(nullptr, false, xworker::CreateWorker());
    auto [task_uid,
          task_future] = xscheduler::ScheduleTask<std::pair<std::string, double>>(scheduler_p.get(), 5, nullptr);

    EXPECT_EQ(xbase::kInvalidUid, task_uid) << "Task uid should be invalid: " << task_uid;
    EXPECT_FALSE(task_future.valid());
}

TEST(xscheduler_test, schedule_non_exists_pf)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, xworker::CreateWorker());
    auto task_uid    = scheduler_p->ScheduleTask(time64::kDay, nullptr);

    EXPECT_EQ(xbase::kInvalidUid, task_uid) << "Task uid should be invalid: " << task_uid;
}
#endif

TEST(xscheduler_test, task_status_of_invalid_uid)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, xworker::CreateWorker());

    auto res = scheduler_p->TaskStatus(xbase::kInvalidUid);
    EXPECT_EQ(xbase::IScheduler::Status::kInvalidUid, res.first) << "Status should be invalid uid.";
    auto empty_task_info = xbase::IScheduler::TaskInfo {};
    EXPECT_EQ(res.second.task_uid, empty_task_info.task_uid);
    EXPECT_EQ(res.second.scheduled_time, empty_task_info.scheduled_time);
    EXPECT_EQ(res.second.scheduling_counter, empty_task_info.scheduling_counter);
    EXPECT_EQ(res.second.worker_busy_counter, empty_task_info.worker_busy_counter);
    EXPECT_EQ(res.second.clock_p, empty_task_info.clock_p);
}

TEST(xscheduler_test, task_cancel_of_invalid_uid)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, xworker::CreateWorker());

    auto res = scheduler_p->CancelTask(xbase::kInvalidUid);
    EXPECT_EQ(xbase::IScheduler::TaskRes::kInvalidUid, res.first) << "Task result should be invalid uid.";
    EXPECT_FALSE(res.second.valid());
}

TEST(xscheduler_test, task_cancel_without_worker)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, nullptr);

    auto task_uid = scheduler_p->ScheduleTask(time64::kDay, [&](const xbase::IScheduler::TaskInfo* _task_info) {
        return time64::kSecond;
    });
    auto res      = scheduler_p->CancelTask(task_uid);
    EXPECT_EQ(xbase::IScheduler::TaskRes::kOk, res.first) << "Task result should be Ok.";
    EXPECT_FALSE(res.second.valid()) << "Task result future should be invalid.";
}

TEST(xscheduler_test, task_cancel_inside_worker)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, true);

    std::promise<void> ready;
    std::atomic<xbase::Uid> task_uid = xbase::kInvalidUid;
    task_uid = scheduler_p->ScheduleTask(time64::kSecond, [&](const xbase::IScheduler::TaskInfo* _task_info) {
        auto [res, future] = scheduler_p->CancelTask(task_uid.load());
        EXPECT_EQ(res, xbase::IScheduler::TaskRes::kOk) << "WRONG STATE";

        EXPECT_FALSE(future.valid());
        ready.set_value();
        return std::nullopt;
    });

    ready.get_future().wait();
}

TEST(xscheduler_test, task_reschedule_of_invalid_uid)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, xworker::CreateWorker());

    auto res = scheduler_p->RescheduleTask(xbase::kInvalidUid, time64::kNoVal);
    EXPECT_EQ(xbase::IScheduler::TaskRes::kInvalidUid, res) << "Task result should be invalid uid.";
}

TEST(xscheduler_test, task_reschedule_of_not_exists_uid)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, xworker::CreateWorker());

    auto res = scheduler_p->RescheduleTask(123123, time64::kNoVal);
    EXPECT_EQ(xbase::IScheduler::TaskRes::kNotFound, res) << "Task result should not be found.";
}

TEST(xscheduler_test, task_schedule_many_tasks)
{
    auto scheduler_p = xscheduler::CreateScheduler(xclock::SysClock(true), false, xworker::CreateWorker({}, 3000, 1));

    size_t                   num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<bool>        was_worker_busy = false;

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&, i = i]() {
            auto task_uid = scheduler_p->ScheduleTask(time64::kSecond * i,
                                                      [&](const xbase::IScheduler::TaskInfo* _task_info) {
                                                          std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                                          return time64::kSecond;
                                                      });
            auto res      = scheduler_p->RescheduleTask(task_uid, time64::kSecond * i + time64::kMisec * i * 100);
            if (res == xbase::IScheduler::TaskRes::kWorkerBusy) {
                was_worker_busy = true;
            }
        });
    }
    std::for_each(threads.begin(), threads.end(), [](auto& th) { th.join(); });
    EXPECT_TRUE(was_worker_busy);
}

TEST(xscheduler_test, basic_task)
{
    std::vector<xbase::IWorker::SPtr> workers = {nullptr, xworker::CreateWorker(), xworker::CreatePool()};
    for (const auto& worker_p : workers) {
        auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, worker_p);

        const std::string expected_res = "check_string";
        double            msec_delay   = 300.0;
        xbase::ClockHR    clock;
        auto [task_uid, task_future] = xscheduler::ScheduleTask<std::pair<std::string, double>>(
            scheduler_p.get(),
            msec_delay,
            [&]() { return std::make_pair(expected_res, clock.TimeMsec()); });

        auto [str, time_msec] = task_future.get();
        EXPECT_EQ(str, expected_res) << "String result is wrong:" << str << " expected:" << expected_res;
        EXPECT_LT(std::abs(time_msec - msec_delay), 100.0)
            << "Executed time is wrong:" << time_msec << " expected:" << msec_delay;
        EXPECT_LT(std::abs(clock.TimeMsec() - msec_delay), 100.0)
            << "Wait time is wrong:" << clock.TimeMsec() << " expected:" << msec_delay;

        // Check what task is gone
        auto res = scheduler_p->CancelTask(task_uid).first;
        EXPECT_NE(res, xbase::IScheduler::TaskRes::kOk) << "Task NOT REMOVED after execution";
    }
}

TEST(xscheduler_test, two_task)
{
    std::vector<xbase::IWorker::SPtr> workers = {nullptr, xworker::CreateWorker(), xworker::CreatePool()};
    for (const auto& worker_p : workers) {

        auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, worker_p);

        const std::string expected_res = "check_string";
        double            msec_delay   = 500.0;
        xbase::ClockHR    clock;
        auto [task_uid, task_future] = xscheduler::ScheduleTask<std::pair<std::string, double>>(
            scheduler_p.get(),
            msec_delay,
            [&]() { return std::make_pair(expected_res, clock.TimeMsec()); });

        auto [task_id2, task_future2] = xscheduler::ScheduleTask<std::pair<std::string, double>>(
            scheduler_p.get(),
            msec_delay,
            [&]() { return std::make_pair(expected_res, clock.TimeMsec()); });

        EXPECT_NE(task_uid, task_id2) << "Diffrent tasks ids is equal:" << task_uid << " & " << task_id2;

        {
            auto [str, time_msec] = task_future.get();
            EXPECT_EQ(str, expected_res) << "String result is wrong:" << str << " expected:" << expected_res;
            EXPECT_LT(std::abs(time_msec - msec_delay), 100.0)
                << "Executed time is wrong:" << time_msec << " expected:" << msec_delay;
            EXPECT_LT(std::abs(clock.TimeMsec() - msec_delay), 100.0)
                << "Wait time is wrong:" << clock.TimeMsec() << " expected:" << msec_delay;
        }

        {
            auto [str, time_msec] = task_future2.get();
            EXPECT_EQ(str, expected_res) << "String result 2 is wrong:" << str << " expected:" << expected_res;
            EXPECT_LT(std::abs(time_msec - msec_delay), 100.0)
                << "Executed time 2 is wrong:" << time_msec << " expected:" << msec_delay;
            EXPECT_LT(std::abs(clock.TimeMsec() - msec_delay), 100.0)
                << "Wait time 2 is wrong:" << clock.TimeMsec() << " expected:" << msec_delay;
        }

        // Check what task is gone
        auto res = scheduler_p->CancelTask(task_uid).first;
        EXPECT_NE(res, xbase::IScheduler::TaskRes::kOk) << "Task NOT REMOVED after execution:" << (uint32_t)res;

        // Check what task 2 is gone
        auto res2 = scheduler_p->CancelTask(task_id2).first;
        EXPECT_NE(res2, xbase::IScheduler::TaskRes::kOk) << "Task 2 NOT REMOVED after execution:" << (uint32_t)res2;
    }
}

TEST(xscheduler_test, two_task_wait)
{
    std::vector<xbase::IWorker::SPtr> workers = {/*nullptr, xworker::CreateWorker(),*/ xworker::CreatePool()};
    for (const auto& worker_p : workers) {

        auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, worker_p);

        const std::string expected_res = "check_string";
        int               msec_delay   = 500;
        xbase::ClockHR    clock;
        auto [task_uid, task_future] = xscheduler::ScheduleTask<std::pair<std::string, double>>(
            scheduler_p.get(),
            time64::kPast,
            [&]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(msec_delay));
                return std::make_pair(expected_res, clock.TimeMsec());
            });

        auto [task_id2, task_future2] = xscheduler::ScheduleTask<std::pair<std::string, double>>(
            scheduler_p.get(),
            time64::kPast,
            [&]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(msec_delay));
                return std::make_pair(expected_res, clock.TimeMsec());
            });

        EXPECT_NE(task_uid, task_id2) << "Different tasks ids is equal:" << task_uid << " & " << task_id2;

        // Sleep for be sure what task started
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Check what task is gone
        auto status = scheduler_p->TaskStatus(task_uid).first;
        EXPECT_EQ(status, xbase::IScheduler::Status::kExecutingNow) << "Task NOT EXECUTED:" << (uint32_t)status;

        // Check what task 2 is gone
        status = scheduler_p->TaskStatus(task_id2).first;
        EXPECT_EQ(status, xbase::IScheduler::Status::kExecutingNow) << "Task 2 NOT EXECUTED:" << (uint32_t)status;

        {
            auto [str, time_msec] = task_future.get();
            EXPECT_EQ(str, expected_res) << "String result is wrong:" << str << " expected:" << expected_res;
            EXPECT_LT(std::abs(time_msec - msec_delay), 100.0)
                << "Executed time is wrong:" << time_msec << " expected:" << msec_delay;
            EXPECT_LT(std::abs(clock.TimeMsec() - msec_delay), 100.0)
                << "Wait time is wrong:" << clock.TimeMsec() << " expected:" << msec_delay;
        }

        {
            auto [str, time_msec] = task_future2.get();
            EXPECT_EQ(str, expected_res) << "String result 2 is wrong:" << str << " expected:" << expected_res;
            EXPECT_LT(std::abs(time_msec - msec_delay), 100.0)
                << "Executed time 2 is wrong:" << time_msec << " expected:" << msec_delay;
            EXPECT_LT(std::abs(clock.TimeMsec() - msec_delay), 100.0)
                << "Wait time 2 is wrong:" << clock.TimeMsec() << " expected:" << msec_delay;
        }

        // Check what task is gone
        status = scheduler_p->TaskStatus(task_uid).first;
        EXPECT_NE(status, xbase::IScheduler::Status::kScheduled)
            << "Task NOT REMOVED after execution:" << (uint32_t)status;

        // Check what task 2 is gone
        status = scheduler_p->TaskStatus(task_id2).first;
        EXPECT_NE(status, xbase::IScheduler::Status::kScheduled)
            << "Task 2 NOT REMOVED after execution:" << (uint32_t)status;

        // Destroy scheduler to wait for worker threads to finish
        scheduler_p->DestroyScheduler();
    }
}

TEST(xscheduler_test, reshedule_task_past)
{
    xbase::IWorker::SPtr worker_p    = xworker::CreateWorker();
    auto                 scheduler_p = xscheduler::CreateScheduler(nullptr, false, worker_p);

    const std::string expected_res = "check_string";
    double            msec_delay   = 1000.0;
    xbase::ClockHR    clock;
    auto [task_uid,
          task_future] = xscheduler::ScheduleTask<std::pair<std::string, double>>(scheduler_p.get(), msec_delay, [&]() {
        EXPECT_EQ(std::this_thread::get_id(), worker_p->ThreadId()) << "Wrong thread context for scheduled task";
        return std::make_pair(expected_res, clock.TimeMsec());
    });

    auto res = scheduler_p->RescheduleTask(task_uid, time64::kPast);
    EXPECT_EQ(res, xbase::IScheduler::TaskRes::kForcedNow) << "RescheduleTask() - not kForcedNow";
    msec_delay = 0;

    auto [str, time_msec] = task_future.get();
    EXPECT_EQ(str, expected_res) << "String result is wrong:" << str << " expected:" << expected_res;
    EXPECT_LT(std::abs(time_msec - msec_delay), 100.0)
        << "Executed time is wrong:" << time_msec << " expected:" << msec_delay;
    EXPECT_LT(std::abs(clock.TimeMsec() - msec_delay), 100.0)
        << "Wait time is wrong:" << clock.TimeMsec() << " expected:" << msec_delay;

    // Check what task is gone
    res = scheduler_p->CancelTask(task_uid).first;
    EXPECT_NE(res, xbase::IScheduler::TaskRes::kOk) << "Task NOT REMOVED after execution:" << (uint32_t)res;
}

TEST(xscheduler_test, reshedule_task_future)
{
    xbase::IWorker::SPtr worker_p    = xworker::CreateWorker();
    auto                 scheduler_p = xscheduler::CreateScheduler(nullptr, false, worker_p);

    const std::string expected_res = "check_string";
    double            msec_delay   = 100.0;
    xbase::ClockHR    clock;
    auto [task_uid,
          task_future] = xscheduler::ScheduleTask<std::pair<std::string, double>>(scheduler_p.get(), msec_delay, [&]() {
        EXPECT_EQ(std::this_thread::get_id(), worker_p->ThreadId()) << "Wrong thread context for scheduled task";
        return std::make_pair(expected_res, clock.TimeMsec());
    });

    msec_delay = 500.0;
    auto res   = scheduler_p->RescheduleTask(task_uid, scheduler_p->Clock()->Time() + time64::FromMsec(msec_delay));
    EXPECT_EQ(res, xbase::IScheduler::TaskRes::kOk) << "RescheduleTask() - not kOk";

    auto [str, time_msec] = task_future.get();
    EXPECT_EQ(str, expected_res) << "String result is wrong:" << str << " expected:" << expected_res;
    EXPECT_LT(std::abs(time_msec - msec_delay), 100.0)
        << "Executed time is wrong:" << time_msec << " expected:" << msec_delay;
    EXPECT_LT(std::abs(clock.TimeMsec() - msec_delay), 100.0)
        << "Wait time is wrong:" << clock.TimeMsec() << " expected:" << msec_delay;

    // Check what task is gone
    res = scheduler_p->CancelTask(task_uid).first;
    EXPECT_NE(res, xbase::IScheduler::TaskRes::kOk) << "Task NOT REMOVED after execution";
}

TEST(xscheduler_test, task_cancel_immediate_repeat)
{
    std::vector<std::pair<bool, xbase::IWorker::SPtr>> default_workers = {{false, nullptr},
                                                                          {true, nullptr},
                                                                          {false, xworker::CreateWorker()},
                                                                          {false, xworker::CreatePool()}};
    for (const auto& [static_pool, default_worker] : default_workers) {

        std::vector<xbase::IWorker::SPtr> task_workers = {nullptr, xworker::CreateWorker()};
        for (const auto& task_worker_p : task_workers) {

            // No task & disabled default worker
            bool no_workers = !task_worker_p && !static_pool && !default_worker;

            auto scheduler_p = xscheduler::CreateScheduler(nullptr, static_pool, default_worker);

            std::promise<void>   started_promise;
            auto                 started  = started_promise.get_future();
            std::atomic_uint64_t counter  = {0};
            auto                 task_uid = scheduler_p->ScheduleTask(
                time64::kPast,
                [&](const auto* _task_info_p) {
                    if (counter.fetch_add(1) == 1)
                        started_promise.set_value();
                    return time64::kPast;
                },
                {},
                task_worker_p);

            ASSERT_NE(task_uid, xbase::kInvalidUid) << "scheduler_p->ScheduleTask() FAILED";

            // Wait for start
            ASSERT_TRUE(started.valid());
            started.wait();

            auto [status, info] = scheduler_p->TaskStatus(task_uid);

            if (no_workers) {
                EXPECT_TRUE(status == xbase::IScheduler::Status::kExecutingNow ||
                            status == xbase::IScheduler::Status::kScheduled);

                EXPECT_GT(info.scheduling_counter, 0)
                    << "wrong TaskInfo::scheduling_counter (for no workers scheduling_counter should be non zero)";
            }
            else {
                EXPECT_EQ(status, xbase::IScheduler::Status::kExecutingNow) << "Task should be executed at this moment";
                EXPECT_EQ(info.scheduling_counter, 0)
                    << "wrong TaskInfo::scheduling_counter (for immediate repeated task should be zero)";
            }

            auto [res, cancel_future] = scheduler_p->CancelTask(task_uid);
            if (no_workers) {
                auto check_counter_0 = counter.load();
                // For task w/o workers wait cancel future is not supported
                ASSERT_FALSE(cancel_future.valid());

                EXPECT_TRUE(res == xbase::IScheduler::TaskRes::kExecutingNow || res == xbase::IScheduler::TaskRes::kOk)
                    << "Task should be executed at this moment or in wait queue";

                if (res == xbase::IScheduler::TaskRes::kExecutingNow) {
                    // Sleep a bit
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    // No more then one cicle after cancel
                    EXPECT_GE(check_counter_0 + 1, counter.load()) << "Task not canceled correctly";
                }
            }
            else {
                EXPECT_EQ(res, xbase::IScheduler::TaskRes::kExecutingNow) << "Task should be executed at this moment";
                auto check_counter_0 = counter.load();

                ASSERT_TRUE(cancel_future.valid());
                cancel_future.wait();
                auto check_counter = counter.load();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                EXPECT_EQ(check_counter, counter.load()) << "Task not canceled correctly";
                EXPECT_GE(check_counter, check_counter_0) << "Wrong counters";
            }
            // Check what task is gone
            auto res2 = scheduler_p->CancelTask(task_uid).first;
            EXPECT_EQ(res2, xbase::IScheduler::TaskRes::kNotFound) << "Task NOT REMOVED after execution";
        }
    }
}

TEST(xscheduler_test, task_cancel_interval_repeats)
{
    const uint32_t interval_msec = 50;
    const uint32_t sleep_msec    = 500;

    std::vector<xbase::IWorker::SPtr> workers = {nullptr, xworker::CreateWorker(), xworker::CreatePool()};
    for (const auto& worker_p : workers) {
        auto scheduler_p = xscheduler::CreateScheduler(xclock::Create(xclock::SteadySyncGen(true), 0).get(),
                                                       false,
                                                       worker_p);

        std::promise<void>   started_promise;
        auto                 started  = started_promise.get_future();
        std::atomic_uint64_t counter  = {0};
        auto                 task_uid = scheduler_p->ScheduleTask(
            time64::kPast,
            [&](const xbase::IScheduler::TaskInfo* _task_info_p) {
                std::cout << "scheduled:" << time64::ToMsec(_task_info_p->scheduled_time)
                          << " now:" << time64::ToMsec(_task_info_p->clock_p->Time())
                          << " delay: " << time64::ToMsec(_task_info_p->clock_p->Time() - _task_info_p->scheduled_time)
                          << " scheduling_counter:" << _task_info_p->scheduling_counter << std::endl;

                if (counter.fetch_add(1) == 1)
                    started_promise.set_value();
                return _task_info_p->scheduled_time + time64::FromMsec(interval_msec);
            },
            {},
            worker_p ? nullptr : xworker::CreateWorker());

        ASSERT_NE(task_uid, xbase::kInvalidUid) << "scheduler_p->ScheduleTask() FAILED";

        // Wait for start
        ASSERT_TRUE(started.valid());
        started.wait();

        auto [status, info] = scheduler_p->TaskStatus(task_uid);
        EXPECT_NE(status, xbase::IScheduler::Status::kNotFound) << "Task should be executed at this moment";
        EXPECT_GT(info.scheduling_counter, 0) << "wrong TaskInfo::scheduling_counter";

        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_msec));

        std::tie(status, info) = scheduler_p->TaskStatus(task_uid);
        EXPECT_NE(status, xbase::IScheduler::Status::kNotFound) << "Task should be executed at this moment";
        // Very estimated counters for test on CI
        EXPECT_GT(info.scheduling_counter + 5, sleep_msec / interval_msec) << "wrong TaskInfo::scheduling_counter";
        EXPECT_LT(info.scheduling_counter, sleep_msec / interval_msec + 5) << "wrong TaskInfo::scheduling_counter";

        auto [res, cancel_future] = scheduler_p->CancelTask(task_uid);
        EXPECT_NE(res, xbase::IScheduler::TaskRes::kNotFound) << "Task should be executed at this moment";
        auto check_counter_0 = counter.load();
        if (cancel_future.valid())
            cancel_future.wait();

        auto check_counter = counter.load();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        EXPECT_EQ(check_counter, counter.load()) << "Task not canceled correctly";
        EXPECT_GE(check_counter, check_counter_0) << "Wrong counters";

        // Check what task is gone
        auto res2 = scheduler_p->CancelTask(task_uid).first;
        EXPECT_EQ(res2, xbase::IScheduler::TaskRes::kNotFound) << "Task NOT REMOVED after execution";
    }

    // EXPECT_FALSE(1);
}

TEST(xscheduler_test, scheduler_destroy)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, nullptr);

    std::atomic<uint64_t> counter;
    auto task_uid = scheduler_p->ScheduleTask(time64::kPast, [&](const xbase::IScheduler::TaskInfo* _task_info) {
        counter.fetch_add(1);
        return time64::kPast;
    });

    auto task_uid2 = scheduler_p->ScheduleTask(time64::kPast, [&](const xbase::IScheduler::TaskInfo* _task_info) {
        counter.fetch_add(1);
        return time64::kPast;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    scheduler_p->DestroyScheduler();

    auto cnt = counter.load();

    auto task_uid_3 = scheduler_p->ScheduleTask(time64::kDay, [&](const xbase::IScheduler::TaskInfo* _task_info) {
        return time64::kSecond;
    });

    EXPECT_EQ(task_uid_3, xbase::kInvalidUid) << "Task added after DestroyScheduler()";

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(cnt, counter.load()) << "Task executed after DestroyScheduler()";
}

TEST(xscheduler_test, scheduled_time_null_scheduler)
{
    EXPECT_EQ(xscheduler::ScheduledTime(nullptr, 1.5), time64::kNoVal);
}

TEST(xscheduler_test, scheduled_time_relative_seconds)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, xworker::CreateWorker());

    const auto now = scheduler_p->Clock()->Time();
    const auto ts  = xscheduler::ScheduledTime(scheduler_p.get(), 1.5);

    EXPECT_GE(ts, now + time64::FromSec(1.4));
    EXPECT_LE(ts, now + time64::FromSec(1.6));
}

TEST(xscheduler_test, is_active_task_invalid_uid)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, xworker::CreateWorker());

    EXPECT_FALSE(xscheduler::IsActiveTask(scheduler_p.get(), xbase::kInvalidUid));
}

TEST(xscheduler_test, is_active_task_scheduled)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, xworker::CreateWorker());

    auto task_uid = scheduler_p->ScheduleTask(time64::kDay, [&](const xbase::IScheduler::TaskInfo* _task_info) {
        return std::nullopt;
    });

    ASSERT_NE(task_uid, xbase::kInvalidUid);

    EXPECT_TRUE(xscheduler::IsActiveTask(scheduler_p.get(), task_uid));

    auto [res, future] = scheduler_p->CancelTask(task_uid);
    EXPECT_EQ(res, xbase::IScheduler::TaskRes::kOk);
    if (future.valid())
        future.wait();
}

TEST(xscheduler_test, is_active_task_finished)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, xworker::CreateWorker());

    auto [task_uid, task_future] = xscheduler::ScheduleTask<int>(scheduler_p.get(), time64::kPast, []() { return 10; });

    ASSERT_NE(task_uid, xbase::kInvalidUid);
    EXPECT_EQ(task_future.get(), 10);

    for (size_t i = 0; i < 100 && xscheduler::IsActiveTask(scheduler_p.get(), task_uid); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_FALSE(xscheduler::IsActiveTask(scheduler_p.get(), task_uid));
}

TEST(xscheduler_test, reschedule_task_no_later_than_invalid_args)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, xworker::CreateWorker());

    EXPECT_FALSE(xscheduler::RescheduleTaskNoLaterThan(nullptr, 123, time64::kSecond).has_value());
    EXPECT_FALSE(
        xscheduler::RescheduleTaskNoLaterThan(scheduler_p.get(), xbase::kInvalidUid, time64::kSecond).has_value());
}

TEST(xscheduler_test, reschedule_task_no_later_than_not_found)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, xworker::CreateWorker());

    EXPECT_FALSE(xscheduler::RescheduleTaskNoLaterThan(scheduler_p.get(), 123456, time64::kSecond).has_value());
}

TEST(xscheduler_test, reschedule_task_no_later_than_keep_existing_time)
{
    xbase::IWorker::SPtr worker_p    = xworker::CreateWorker();
    auto                 scheduler_p = xscheduler::CreateScheduler(nullptr, false, worker_p);

    const auto initial_time = scheduler_p->Clock()->Time() + time64::FromMsec(100.0);
    auto       task_uid = scheduler_p->ScheduleTask(initial_time, [&](const xbase::IScheduler::TaskInfo* _task_info) {
        return std::nullopt;
    });

    ASSERT_NE(task_uid, xbase::kInvalidUid);

    const auto later_time     = scheduler_p->Clock()->Time() + time64::FromMsec(500.0);
    auto       effective_time = xscheduler::RescheduleTaskNoLaterThan(scheduler_p.get(), task_uid, later_time);

    ASSERT_TRUE(effective_time.has_value());
    EXPECT_EQ(*effective_time, initial_time);

    auto [status, info] = scheduler_p->TaskStatus(task_uid);
    EXPECT_EQ(status, xbase::IScheduler::Status::kScheduled);
    EXPECT_EQ(info.scheduled_time, initial_time);

    auto [res, future] = scheduler_p->CancelTask(task_uid);
    EXPECT_EQ(res, xbase::IScheduler::TaskRes::kOk);
    if (future.valid())
        future.wait();
}

TEST(xscheduler_test, reschedule_task_no_later_than_move_earlier)
{
    xbase::IWorker::SPtr worker_p    = xworker::CreateWorker();
    auto                 scheduler_p = xscheduler::CreateScheduler(nullptr, false, worker_p);

    const auto late_time = scheduler_p->Clock()->Time() + time64::FromMsec(1000.0);
    auto       task_uid  = scheduler_p->ScheduleTask(late_time, [&](const xbase::IScheduler::TaskInfo* _task_info) {
        return std::nullopt;
    });

    ASSERT_NE(task_uid, xbase::kInvalidUid);

    const auto earlier_time   = scheduler_p->Clock()->Time() + time64::FromMsec(100.0);
    auto       effective_time = xscheduler::RescheduleTaskNoLaterThan(scheduler_p.get(), task_uid, earlier_time);

    ASSERT_TRUE(effective_time.has_value());
    EXPECT_EQ(*effective_time, earlier_time);

    auto [status, info] = scheduler_p->TaskStatus(task_uid);
    EXPECT_EQ(status, xbase::IScheduler::Status::kScheduled);
    EXPECT_EQ(info.scheduled_time, earlier_time);

    auto [res, future] = scheduler_p->CancelTask(task_uid);
    EXPECT_EQ(res, xbase::IScheduler::TaskRes::kOk);
    if (future.valid())
        future.wait();
}

TEST(xscheduler_test, reschedule_task_no_later_than_executing_now)
{
    xbase::IWorker::SPtr worker_p    = xworker::CreateWorker();
    auto                 scheduler_p = xscheduler::CreateScheduler(nullptr, false, worker_p);

    std::promise<void> started_promise;
    auto               started = started_promise.get_future();

    auto task_uid = scheduler_p->ScheduleTask(time64::kPast, [&](const xbase::IScheduler::TaskInfo* _task_info) {
        started_promise.set_value();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        return std::nullopt;
    });

    ASSERT_NE(task_uid, xbase::kInvalidUid);

    started.wait();

    auto [status_before, info_before] = scheduler_p->TaskStatus(task_uid);
    ASSERT_EQ(status_before, xbase::IScheduler::Status::kExecutingNow);

    auto effective_time = xscheduler::RescheduleTaskNoLaterThan(scheduler_p.get(),
                                                                task_uid,
                                                                scheduler_p->Clock()->Time());

    ASSERT_TRUE(effective_time.has_value());
    EXPECT_EQ(*effective_time, info_before.scheduled_time);

    auto [res, future] = scheduler_p->CancelTask(task_uid);
    EXPECT_EQ(res, xbase::IScheduler::TaskRes::kExecutingNow);
    if (future.valid())
        future.wait();
}

TEST(xscheduler_test, run_task_no_later_than_schedule_new)
{
    xbase::IWorker::SPtr worker_p    = xworker::CreateWorker();
    auto                 scheduler_p = xscheduler::CreateScheduler(nullptr, false, worker_p);

    std::atomic<xbase::Uid> atomic_task_uid = xbase::kInvalidUid;

    const auto scheduled_time = scheduler_p->Clock()->Time() + time64::FromMsec(300.0);
    auto       res            = xscheduler::RunTaskNoLaterThan(
        atomic_task_uid,
        scheduler_p.get(),
        scheduled_time,
        [&](const xbase::IScheduler::TaskInfo* _task_info) { return std::nullopt; });

    EXPECT_EQ(res.status, xscheduler::ScheduleTaskStatus::kScheduledNew);
    EXPECT_NE(res.active_task_uid, xbase::kInvalidUid);
    EXPECT_EQ(atomic_task_uid.load(), res.active_task_uid);

    auto [status, info] = scheduler_p->TaskStatus(res.active_task_uid);
    EXPECT_EQ(status, xbase::IScheduler::Status::kScheduled);
    EXPECT_EQ(info.scheduled_time, scheduled_time);

    auto [cancel_res, future] = scheduler_p->CancelTask(res.active_task_uid);
    EXPECT_EQ(cancel_res, xbase::IScheduler::TaskRes::kOk);
    if (future.valid())
        future.wait();
}

TEST(xscheduler_test, run_task_no_later_than_existing_task_already_active)
{
    xbase::IWorker::SPtr worker_p    = xworker::CreateWorker();
    auto                 scheduler_p = xscheduler::CreateScheduler(nullptr, false, worker_p);

    std::atomic<xbase::Uid> atomic_task_uid = xbase::kInvalidUid;

    const auto first_time = scheduler_p->Clock()->Time() + time64::FromMsec(100.0);
    auto       first_res  = xscheduler::RunTaskNoLaterThan(
        atomic_task_uid,
        scheduler_p.get(),
        first_time,
        [&](const xbase::IScheduler::TaskInfo* _task_info) { return std::nullopt; });

    ASSERT_EQ(first_res.status, xscheduler::ScheduleTaskStatus::kScheduledNew);
    ASSERT_NE(first_res.active_task_uid, xbase::kInvalidUid);

    const auto later_time = scheduler_p->Clock()->Time() + time64::FromMsec(500.0);
    auto       second_res = xscheduler::RunTaskNoLaterThan(
        atomic_task_uid,
        scheduler_p.get(),
        later_time,
        [&](const xbase::IScheduler::TaskInfo* _task_info) { return std::nullopt; });

    EXPECT_EQ(second_res.status, xscheduler::ScheduleTaskStatus::kAlreadyActive);
    EXPECT_EQ(second_res.active_task_uid, first_res.active_task_uid);
    EXPECT_EQ(atomic_task_uid.load(), first_res.active_task_uid);

    auto [status, info] = scheduler_p->TaskStatus(first_res.active_task_uid);
    EXPECT_EQ(status, xbase::IScheduler::Status::kScheduled);
    EXPECT_EQ(info.scheduled_time, first_time);

    auto [cancel_res, future] = scheduler_p->CancelTask(first_res.active_task_uid);
    EXPECT_EQ(cancel_res, xbase::IScheduler::TaskRes::kOk);
    if (future.valid())
        future.wait();
}

TEST(xscheduler_test, run_task_no_later_than_existing_task_rescheduled_earlier)
{
    xbase::IWorker::SPtr worker_p    = xworker::CreateWorker();
    auto                 scheduler_p = xscheduler::CreateScheduler(nullptr, false, worker_p);

    std::atomic<xbase::Uid> atomic_task_uid = xbase::kInvalidUid;

    const auto late_time = scheduler_p->Clock()->Time() + time64::FromMsec(1000.0);
    auto       first_res = xscheduler::RunTaskNoLaterThan(
        atomic_task_uid,
        scheduler_p.get(),
        late_time,
        [&](const xbase::IScheduler::TaskInfo* _task_info) { return std::nullopt; });

    ASSERT_EQ(first_res.status, xscheduler::ScheduleTaskStatus::kScheduledNew);
    ASSERT_NE(first_res.active_task_uid, xbase::kInvalidUid);

    const auto earlier_time = scheduler_p->Clock()->Time() + time64::FromMsec(100.0);
    auto       second_res   = xscheduler::RunTaskNoLaterThan(
        atomic_task_uid,
        scheduler_p.get(),
        earlier_time,
        [&](const xbase::IScheduler::TaskInfo* _task_info) { return std::nullopt; });

    EXPECT_EQ(second_res.status, xscheduler::ScheduleTaskStatus::kAlreadyActive);
    EXPECT_EQ(second_res.active_task_uid, first_res.active_task_uid);
    EXPECT_EQ(atomic_task_uid.load(), first_res.active_task_uid);

    auto [status, info] = scheduler_p->TaskStatus(first_res.active_task_uid);
    EXPECT_EQ(status, xbase::IScheduler::Status::kScheduled);
    EXPECT_EQ(info.scheduled_time, earlier_time);

    auto [cancel_res, future] = scheduler_p->CancelTask(first_res.active_task_uid);
    EXPECT_EQ(cancel_res, xbase::IScheduler::TaskRes::kOk);
    if (future.valid())
        future.wait();
}

TEST(xscheduler_test, run_task_no_later_than_existing_task_executing)
{
    xbase::IWorker::SPtr worker_p    = xworker::CreateWorker();
    auto                 scheduler_p = xscheduler::CreateScheduler(nullptr, false, worker_p);

    std::atomic<xbase::Uid> atomic_task_uid = xbase::kInvalidUid;
    std::promise<void>      started_promise;
    auto                    started = started_promise.get_future();

    const auto task_uid = scheduler_p->ScheduleTask(time64::kPast, [&](const xbase::IScheduler::TaskInfo* _task_info) {
        started_promise.set_value();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        return std::nullopt;
    });

    ASSERT_NE(task_uid, xbase::kInvalidUid);
    atomic_task_uid.store(task_uid);

    started.wait();

    auto res = xscheduler::RunTaskNoLaterThan(
        atomic_task_uid,
        scheduler_p.get(),
        scheduler_p->Clock()->Time(),
        [&](const xbase::IScheduler::TaskInfo* _task_info) { return std::nullopt; });

    EXPECT_EQ(res.status, xscheduler::ScheduleTaskStatus::kAlreadyActive);
    EXPECT_EQ(res.active_task_uid, task_uid);
    EXPECT_EQ(atomic_task_uid.load(), task_uid);

    auto [cancel_res, future] = scheduler_p->CancelTask(task_uid);
    EXPECT_EQ(cancel_res, xbase::IScheduler::TaskRes::kExecutingNow);
    if (future.valid())
        future.wait();
}

TEST(xscheduler_test, stop_task_empty_atomic)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, xworker::CreateWorker());

    std::atomic<xbase::Uid> atomic_task_uid = xbase::kInvalidUid;

    auto task_uid = xscheduler::StopTask(atomic_task_uid, scheduler_p.get(), false);

    EXPECT_EQ(task_uid, xbase::kInvalidUid);
}

TEST(xscheduler_test, stop_task_cancel_scheduled_task)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, xworker::CreateWorker());

    std::atomic<xbase::Uid> atomic_task_uid = xbase::kInvalidUid;

    auto task_uid = scheduler_p->ScheduleTask(time64::kDay, [&](const xbase::IScheduler::TaskInfo* _task_info) {
        return std::nullopt;
    });
    ASSERT_NE(task_uid, xbase::kInvalidUid);

    atomic_task_uid.store(task_uid);

    auto stopped_uid = xscheduler::StopTask(atomic_task_uid, scheduler_p.get(), false);

    EXPECT_EQ(stopped_uid, task_uid);
    EXPECT_EQ(atomic_task_uid.load(), xbase::kInvalidUid);
    EXPECT_EQ(scheduler_p->TaskStatus(task_uid).first, xbase::IScheduler::Status::kNotFound);
}

TEST(xscheduler_test, stop_task_wait_for_finish)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, false, xworker::CreateWorker());

    std::atomic<xbase::Uid> atomic_task_uid = xbase::kInvalidUid;
    std::atomic<bool>       finished        = false;
    std::promise<void>      started_promise;
    auto                    started = started_promise.get_future();

    auto task_uid = scheduler_p->ScheduleTask(time64::kPast, [&](const xbase::IScheduler::TaskInfo* _task_info) {
        started_promise.set_value();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        finished = true;
        return time64::kPast;
    });
    ASSERT_NE(task_uid, xbase::kInvalidUid);

    atomic_task_uid.store(task_uid);
    started.wait();

    auto stopped_uid = xscheduler::StopTask(atomic_task_uid, scheduler_p.get(), true);

    EXPECT_EQ(stopped_uid, task_uid);
    EXPECT_EQ(atomic_task_uid.load(), xbase::kInvalidUid);
    EXPECT_TRUE(finished.load());

    auto status = scheduler_p->TaskStatus(task_uid).first;
    EXPECT_EQ(status, xbase::IScheduler::Status::kNotFound);
}
