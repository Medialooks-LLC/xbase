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
    auto scheduler_p   = xscheduler::CreateScheduler(nullptr, xworker::CreateWorker());
    auto [task_uid,
          task_future] = xscheduler::ScheduleTask<std::pair<std::string, double>>(scheduler_p.get(), 5, nullptr);

    EXPECT_EQ(xbase::kInvalidUid, task_uid) << "Task uid should be invalid: " << task_uid;
    EXPECT_FALSE(task_future.valid());
}

TEST(xscheduler_test, schedule_non_exists_pf)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, xworker::CreateWorker());
    auto task_uid    = scheduler_p->ScheduleTask(time64::kDay, nullptr);

    EXPECT_EQ(xbase::kInvalidUid, task_uid) << "Task uid should be invalid: " << task_uid;
}
#endif

TEST(xscheduler_test, task_status_of_invalid_uid)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, xworker::CreateWorker());

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
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, xworker::CreateWorker());

    auto res = scheduler_p->CancelTask(xbase::kInvalidUid);
    EXPECT_EQ(xbase::IScheduler::TaskRes::kInvalidUid, res.first) << "Task result should be invalid uid.";
    EXPECT_FALSE(res.second.valid());
}

TEST(xscheduler_test, task_cancel_without_worker)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, nullptr);

    auto task_uid = scheduler_p->ScheduleTask(time64::kDay, [&](const xbase::IScheduler::TaskInfo* _task_info) {
        return time64::kSecond;
    });
    auto res      = scheduler_p->CancelTask(task_uid);
    EXPECT_EQ(xbase::IScheduler::TaskRes::kOk, res.first) << "Task result should be Ok.";
    EXPECT_FALSE(res.second.valid()) << "Task result future should be invalid.";
}

TEST(xscheduler_test, task_reschedule_of_invalid_uid)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, xworker::CreateWorker());

    auto res = scheduler_p->RescheduleTask(xbase::kInvalidUid, time64::kNoVal);
    EXPECT_EQ(xbase::IScheduler::TaskRes::kInvalidUid, res) << "Task result should be invalid uid.";
}

TEST(xscheduler_test, task_reschedule_of_not_exists_uid)
{
    auto scheduler_p = xscheduler::CreateScheduler(nullptr, xworker::CreateWorker());

    auto res = scheduler_p->RescheduleTask(123123, time64::kNoVal);
    EXPECT_EQ(xbase::IScheduler::TaskRes::kNotFound, res) << "Task result should not be found.";
}

TEST(xscheduler_test, task_schedule_many_tasks)
{
    auto scheduler_p = xscheduler::CreateScheduler(xclock::SysClock(true), xworker::CreateWorker({}, 3000, 1));

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
        auto scheduler_p = xscheduler::CreateScheduler(nullptr, worker_p);

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

        auto scheduler_p = xscheduler::CreateScheduler(nullptr, worker_p);

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

        auto scheduler_p = xscheduler::CreateScheduler(nullptr, worker_p);

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
    }
}

TEST(xscheduler_test, reshedule_task_past)
{
    xbase::IWorker::SPtr worker_p    = xworker::CreateWorker();
    auto                 scheduler_p = xscheduler::CreateScheduler(nullptr, worker_p);

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
    auto                 scheduler_p = xscheduler::CreateScheduler(nullptr, worker_p);

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
    std::vector<xbase::IWorker::SPtr> workers = {nullptr, xworker::CreateWorker(), xworker::CreatePool()};
    for (const auto& worker_p : workers) {
        auto scheduler_p = xscheduler::CreateScheduler(nullptr, worker_p);

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
            worker_p ? nullptr : xworker::CreateWorker());

        ASSERT_NE(task_uid, xbase::kInvalidUid) << "scheduler_p->ScheduleTask() FAILED";

        // Wait for start
        ASSERT_TRUE(started.valid());
        started.wait();

        auto [status, info] = scheduler_p->TaskStatus(task_uid);
        EXPECT_EQ(status, xbase::IScheduler::Status::kExecutingNow) << "Task should be executed at this moment";
        EXPECT_EQ(info.scheduling_counter, 0) << "wrong TaskInfo::scheduling_counter";

        auto [res, cancel_future] = scheduler_p->CancelTask(task_uid);
        EXPECT_EQ(res, xbase::IScheduler::TaskRes::kExecutingNow) << "Task should be executed at this moment";
        auto check_counter_0 = counter.load();
        ASSERT_TRUE(cancel_future.valid());
        cancel_future.wait();
        auto check_counter = counter.load();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        EXPECT_EQ(check_counter, counter.load()) << "Task not canceled correctly";
        EXPECT_GE(check_counter, check_counter_0) << "Wrong counters";

        // Check what task is gone
        auto res2 = scheduler_p->CancelTask(task_uid).first;
        EXPECT_EQ(res2, xbase::IScheduler::TaskRes::kNotFound) << "Task NOT REMOVED after execution";
    }
}

TEST(xscheduler_test, task_cancel_interval_repeats)
{
    const uint32_t interval_msec = 50;
    const uint32_t sleep_msec    = 500;

    std::vector<xbase::IWorker::SPtr> workers = {nullptr, xworker::CreateWorker(), xworker::CreatePool()};
    for (const auto& worker_p : workers) {
        auto scheduler_p = xscheduler::CreateScheduler(xclock::Create(xclock::SteadySyncGen(true), 0).get(), worker_p);

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
