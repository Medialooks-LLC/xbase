#include "xbase.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <future>
#include <thread>

using namespace xsdk;

class FakeClass {
    uint64_t                 test_ = 0;
    std::array<uint64_t, 64> array_;

public:
    explicit FakeClass(uint64_t _test) { Set(_test); }
    void Set(uint64_t _test)
    {
        test_ = _test;
        for (auto& val : array_)
            val = ++test_;
    }
};

// We disable assert definition, because these tests should be run only while collecting code coverage or in release
// mode
#ifdef NDEBUG
TEST(xworker_tests, execute_async_tmpl_wo_worker)
{
    const std::string expected_res = "check_string";
    auto              task_future  = xworker::ExecuteAsync<std::string>(nullptr, [&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return expected_res;
    });

    EXPECT_FALSE(task_future.valid());
}

TEST(xworker_tests, execute_async_wo_worker)
{
    auto task_future = xworker::ExecuteAsync(nullptr, []() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return xbase::IWorker::RepeatType::kDoNotRepeat;
    });

    EXPECT_FALSE(task_future.valid());
}

TEST(xworker_tests, execute_async_tmpl_wo_pf)
{
    xbase::ClockHR clock;
    auto           task_future = xworker::ExecuteAsync<std::string>(xworker::CreateWorker().get(), nullptr);

    EXPECT_FALSE(task_future.valid());
}

TEST(xworker_tests, execute_async_wo_pf)
{
    xbase::ClockHR clock;
    auto           task_future = xworker::ExecuteAsync(xworker::CreateWorker().get(), nullptr);

    EXPECT_FALSE(task_future.valid());
}

TEST(xworker_tests, execute_sync_wo_pf)
{
    xbase::ClockHR clock;
    auto           task_res = xworker::ExecuteSync(xworker::CreateWorker().get(), nullptr);

    EXPECT_FALSE(task_res);
}

TEST(xworker_tests, worker_execute_async_task_readd_canceled_task)
{
    auto          worker_p   = xworker::CreateWorker({}, {}, 10);
    const int32_t delay_msec = 500;
    auto          task_id    = 321321;
    auto          promise_sp = std::make_shared<std::promise<bool>>();
    auto          future     = promise_sp->get_future();

    xworker::ExecuteAsync(
        worker_p.get(),
        [&]() {
            promise_sp->set_value(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
            return xbase::IWorker::RepeatType::kDoNotRepeat;
        },
        task_id);
    [[maybe_unused]] auto val    = future.get();
    auto                  cancel = worker_p->TaskCancel(task_id);
    EXPECT_EQ(cancel.first, xbase::IWorker::CancelRes::kExecutingNow) << "Cancel valid task return strange res";
    auto res = xworker::ExecuteAsync(
        worker_p.get(),
        [&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
            return xbase::IWorker::RepeatType::kDoNotRepeat;
        },
        task_id);

    EXPECT_FALSE(res.valid()) << "Can add canceled task again to worker";
}

TEST(xworker_tests, worker_execute_sync__task_readd_canceled_task)
{
    auto          worker_p   = xworker::CreateWorker({}, {}, 10);
    const int32_t delay_msec = 500;
    auto          task_id    = 321321;
    auto          promise_sp = std::make_shared<std::promise<bool>>();
    auto          future     = promise_sp->get_future();

    std::thread           th([&]() {
        xworker::ExecuteSync(
            worker_p.get(),
            [&]() {
                promise_sp->set_value(true);
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
                return xbase::IWorker::RepeatType::kDoNotRepeat;
            },
            {},
            task_id);
    });
    [[maybe_unused]] auto val    = future.get();
    auto                  cancel = worker_p->TaskCancel(task_id);
    EXPECT_EQ(cancel.first, xbase::IWorker::CancelRes::kExecutingNow) << "Cancel valid task return strange res";
    auto res = xworker::ExecuteSync(
        worker_p.get(),
        [&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
            return xbase::IWorker::RepeatType::kDoNotRepeat;
        },
        {},
        task_id);

    EXPECT_FALSE(res) << "Can add canceled task again to worker";
    th.join();
}
#endif

TEST(xworker_tests, pool_add_task_requred_idle_state_to_busy_pool)
{
    auto          worker_p   = xworker::CreatePool(1, 1);
    const int32_t delay_msec = 500;
    auto          task_id    = 321321;
    auto          promise_sp = std::make_shared<std::promise<bool>>();
    auto          future     = promise_sp->get_future();

    xworker::ExecuteAsync(
        worker_p.get(),
        [&]() {
            promise_sp->set_value(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
            return xbase::IWorker::RepeatType::kDoNotRepeat;
        },
        task_id);
    [[maybe_unused]] auto val    = future.get();
    auto                  cancel = worker_p->TaskCancel(task_id);
    EXPECT_EQ(cancel.first, xbase::IWorker::CancelRes::kExecutingNow) << "Cancel valid task return strange res";
    auto res = xworker::ExecuteAsync(
        worker_p.get(),
        [&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
            return xbase::IWorker::RepeatType::kDoNotRepeat;
        },
        task_id,
        xbase::IWorker::State::kIdle);

    EXPECT_FALSE(res.valid()) << "Can add canceled task again to worker";
}

TEST(xworker_tests, execute_sync_wo_worker)
{
    auto task_res = xworker::ExecuteSync(nullptr, []() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return xbase::IWorker::RepeatType::kDoNotRepeat;
    });

    EXPECT_TRUE(task_res);
}

TEST(xworker_tests, execute_async_res_with_bad_task)
{
    auto worker_p = xworker::CreateWorker({}, {}, 0);

    const int32_t     delay_msec    = 1000;
    const std::string expected_res  = "check_string";
    auto              task_future_1 = xworker::ExecuteAsync<std::string>(worker_p.get(), [&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
        return expected_res;
    });

    auto task_future = xworker::ExecuteAsync<std::string>(worker_p.get(), [&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
        return expected_res;
    });

    ASSERT_FALSE(task_future.valid());
}

TEST(xworker_tests, pool_try_add_more_tasks_then_max)
{
    auto pool_p = xworker::CreatePool(1, 1, 3000, 0);

    const int32_t delay_msec = 500;
    pool_p->TaskPut([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
        return xbase::IWorker::RepeatType::kDoNotRepeat;
    });

    auto task_id = pool_p->TaskPut([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
        return xbase::IWorker::RepeatType::kDoNotRepeat;
    });

    ASSERT_EQ(task_id, xbase::kInvalidUid);
}

TEST(xworker_tests, pool_try_check_preffered_workers) // test used only for code coverage and should not crashed
{
    auto pool_p = xworker::CreatePool(4, 8, 30 * 1000, 0);

    const int32_t delay_msec = 500;
    pool_p->TaskPut([&]() { return xbase::IWorker::RepeatType::kDoNotRepeat; }, 444411);
    pool_p->TaskPut([&]() { return xbase::IWorker::RepeatType::kDoNotRepeat; }, 444411);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
    pool_p->TaskPut([&]() { return xbase::IWorker::RepeatType::kDoNotRepeat; }, 444411);
    std::this_thread::sleep_for(std::chrono::milliseconds(62 * 1000));
    pool_p->TaskPut([&]() { return xbase::IWorker::RepeatType::kDoNotRepeat; }, 333311);
    pool_p->TaskPut([&]() { return xbase::IWorker::RepeatType::kDoNotRepeat; }, 444411);
}

TEST(xworker_tests, worker_tasks_count)
{
    auto worker_p = xworker::CreateWorker({}, {}, 10);
    ASSERT_EQ(worker_p->TasksCount().first, 0);
    ASSERT_EQ(worker_p->TasksCount().second, 0);

    const int32_t     delay_msec   = 500;
    const std::string expected_res = "check_string";
    auto              promise_sp   = std::make_shared<std::promise<bool>>();
    auto              future       = promise_sp->get_future();
    xworker::ExecuteAsync<std::string>(worker_p.get(), [&]() {
        promise_sp->set_value(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
        return expected_res;
    });
    xworker::ExecuteAsync<std::string>(worker_p.get(), [&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
        return expected_res;
    });
    xworker::ExecuteAsync<std::string>(worker_p.get(), [&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
        return expected_res;
    });
    [[maybe_unused]] auto val         = future.get();
    auto                  count_tasks = worker_p->TasksCount();

    EXPECT_EQ(count_tasks.first, 1) << "TasksCount: " << count_tasks.first << ", " << count_tasks.second;
    EXPECT_TRUE(count_tasks.second > 1) << "TasksCount: " << count_tasks.first << ", " << count_tasks.second;
}

TEST(xworker_tests, pool_tasks_count)
{
    auto pool_p = xworker::CreatePool(4, 8);
    ASSERT_EQ(pool_p->TasksCount().first, 0);
    ASSERT_EQ(pool_p->TasksCount().second, 0);

    const int32_t     delay_msec   = 500;
    const std::string expected_res = "check_string";

    for (int i = 0; i < 100; i++) {
        xworker::ExecuteAsync<std::string>(pool_p.get(), [&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
            return expected_res;
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    auto count_tasks = pool_p->TasksCount();
    EXPECT_EQ(count_tasks.first, 8) << "TasksCount: " << count_tasks.first << ", " << count_tasks.second;
    EXPECT_TRUE(count_tasks.second > 1) << "TasksCount: " << count_tasks.first << ", " << count_tasks.second;
}

TEST(xworker_tests, worker_task_cancel_invalid_uid)
{
    auto worker_p = xworker::CreateWorker({}, {}, 10);

    auto res = worker_p->TaskCancel(xbase::kInvalidUid);
    EXPECT_EQ(res.first, xbase::IWorker::CancelRes::kInvalidUid) << "Cancel invalid task return strange res";
}

TEST(xworker_tests, pool_task_cancel_invalid_uid)
{
    auto pool_p = xworker::CreatePool(4, 8);

    auto res = pool_p->TaskCancel(xbase::kInvalidUid);
    EXPECT_EQ(res.first, xbase::IWorker::CancelRes::kInvalidUid) << "Cancel invalid task return strange res";
}

TEST(xworker_tests, pool_task_cancel_not_exists_uid)
{
    auto pool_p = xworker::CreatePool(4, 8);

    auto res = pool_p->TaskCancel(123321);
    EXPECT_EQ(res.first, xbase::IWorker::CancelRes::kNotFound) << "Cancel not exists task return strange res";
}

TEST(xworker_tests, worker_task_cancel_valid_uid)
{
    auto worker_p = xworker::CreateWorker({}, {}, 10);

    const int32_t     delay_msec   = 500;
    const std::string expected_res = "check_string";
    xworker::ExecuteAsync<std::string>(
        worker_p.get(),
        [&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
            return expected_res;
        },
        123123);

    auto res = worker_p->TaskCancel(123123);
    EXPECT_EQ(res.first, xbase::IWorker::CancelRes::kCanceled) << "Cancel valid task return strange res";
}

TEST(xworker_tests, pool_task_cancel_valid_uid)
{
    auto pool_p = xworker::CreatePool(1, 1);

    const int32_t     delay_msec   = 500;
    const std::string expected_res = "check_string";
    xworker::ExecuteAsync<std::string>(pool_p.get(), [&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
        return expected_res;
    });
    xworker::ExecuteAsync<std::string>(
        pool_p.get(),
        [&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
            return expected_res;
        },
        123123);

    auto res = pool_p->TaskCancel(123123);
    EXPECT_EQ(res.first, xbase::IWorker::CancelRes::kCanceled) << "Cancel valid task return strange res";
}

TEST(xworker_tests, worker_get_max_tasks)
{
    auto worker_p = xworker::CreateWorker({}, {}, 2);
    ASSERT_EQ(2, worker_p->MaxTasks()) << "Wrong max tasks number";
}

TEST(xworker_tests, pool_get_max_tasks)
{
    auto pool_p = xworker::CreatePool(2, 4, 500, 8);
    ASSERT_EQ(8, pool_p->MaxTasks()) << "Wrong max tasks number";
}

TEST(xworker_tests, execute_async_res)
{
    auto pool_p = xworker::CreatePool(4, 8);

    const int32_t     delay_msec   = 500;
    const std::string expected_res = "check_string";
    xbase::ClockHR    clock;
    auto              task_future = xworker::ExecuteAsync<std::string>(pool_p.get(), [&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_msec));
        clock.ResetLap();
        return expected_res;
    });

    ASSERT_TRUE(task_future.valid());
    auto res = task_future.get();
    EXPECT_GE(clock.TimeMsec(), delay_msec) << "Wrong wait time";
    EXPECT_LE(clock.Lap(), time64::FromMsec(15))
        << "Wrong time after function executed:" << time64::ToMsec(clock.Lap());
    EXPECT_EQ(res, expected_res) << "Wrong string res";
}

TEST(xworker_tests, pool_tasks_cancel)
{
    std::array<std::atomic_uint64_t, 16> counters = {};
    std::srand((uint32_t)std::time(nullptr));

    auto pool_task_pf = [&](const size_t idx) {
        [[maybe_unused]] auto v = counters[idx].fetch_add(1);
        if (idx % 2) {
            auto sleep_msec = std::rand() % 500;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_msec));
        }
        return xbase::IWorker::RepeatType::kRepeatUntilCancel;
    };

    auto pool_p = xworker::CreatePool(0, counters.size(), 300);

    std::vector<xbase::IWorker::TaskUid> task_uids;
    for (size_t z = 0; z < counters.size(); ++z) {
        auto task_uid = pool_p->TaskPut([=]() { return pool_task_pf(z); });
        ASSERT_TRUE(task_uid != xbase::kInvalidUid) << "pool_p->TaskPut FAILED";

        if (task_uid != xbase::kInvalidUid)
            task_uids.push_back(task_uid);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto [exec, total] = pool_p->TasksCount();
    EXPECT_EQ(exec, counters.size()) << "Wrong executing task counter";
    EXPECT_EQ(total, 0) << "Wrong await task counter";

    std::array<std::uint64_t, 16> check_counters = {};
    for (size_t z = 0; z < task_uids.size(); ++z) {
        auto [res, cancel_future] = pool_p->TaskCancel(task_uids[z]);
        EXPECT_EQ(res, xbase::IWorker::CancelRes::kExecutingNow);
        if (cancel_future.valid())
            cancel_future.wait();
        check_counters[z] = counters[z].load();
    }

    // Future is set before number of tasks decreased
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::tie(exec, total) = pool_p->TasksCount();
    EXPECT_EQ(exec, 0) << "Wrong executing task counter (after cancel)";
    EXPECT_EQ(total, 0) << "Wrong total task counter (after cancel)";

    // Check for no cicles after stop
    for (size_t z = 0; z < task_uids.size(); ++z) {
        EXPECT_EQ(counters[z].load(), check_counters[z]) << "Some cicles after cancel";
    }
}

TEST(xworker_tests, pool_tasks_cancel_with_deq)
{
    std::array<std::atomic_uint64_t, 16> counters = {};
    std::srand((uint32_t)std::time(nullptr));

    auto pool_task_pf = [&](const size_t idx) {
        [[maybe_unused]] auto v = counters[idx].fetch_add(1);
        if (idx % 2) {
            auto sleep_msec = std::rand() % 500;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_msec));
        }
        return xbase::IWorker::RepeatType::kRepeatUntilCancel;
    };

    auto pool_p = xworker::CreatePool(0, counters.size() / 2, 300);

    std::vector<xbase::IWorker::TaskUid> task_uids;
    for (size_t z = 0; z < counters.size(); ++z) {
        auto task_uid = pool_p->TaskPut([=]() { return pool_task_pf(z); });
        ASSERT_TRUE(task_uid != xbase::kInvalidUid) << "pool_p->TaskPut FAILED";

        if (task_uid != xbase::kInvalidUid)
            task_uids.push_back(task_uid);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto [exec, total] = pool_p->TasksCount();
    EXPECT_EQ(exec, counters.size() / 2) << "Wrong executing task counter";
    EXPECT_EQ(exec + total, counters.size()) << "Wrong executing/total task counter";

    size_t                        executing      = 0;
    std::array<std::uint64_t, 16> check_counters = {};
    for (size_t z = 0; z < task_uids.size(); ++z) {

        auto [res, cancel_future] = pool_p->TaskCancel(task_uids[z]);

        if (cancel_future.valid()) {
            EXPECT_EQ(res, xbase::IWorker::CancelRes::kExecutingNow);
            cancel_future.wait();
            ++executing;
        }
        else {
            EXPECT_EQ(res, xbase::IWorker::CancelRes::kCanceled);
        }
        check_counters[z] = counters[z].load();
    }

    EXPECT_GE(executing, exec) << "Wrong number of execiting tasks";

    // Future is set before number of tasks decreased
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::tie(exec, total) = pool_p->TasksCount();
    EXPECT_EQ(exec, 0) << "Wrong executing task counter (after cancel)";
    EXPECT_EQ(total, 0) << "Wrong total task counter (after cancel)";

    // Check for no cicles after stop
    for (size_t z = 0; z < task_uids.size(); ++z) {
        EXPECT_EQ(counters[z].load(), check_counters[z]) << "Some cicles after cancel";
    }
}

TEST(xworker_tests, pool_execute_sync)
{
    volatile bool                        stop     = false;
    std::array<std::atomic_uint64_t, 16> counters = {};
    std::vector<std::string>             descs;

    auto pool_p = xworker::CreatePool(0, 8, 300);

    std::atomic_int64_t shared       = {};
    auto                do_something = [&](std::atomic_uint64_t& val) {
        auto v      = val.fetch_add(1);
        auto test_p = std::make_shared<FakeClass>(v);
        auto s      = shared.fetch_add(1);
        test_p->Set(s);
    };

    std::vector<std::thread> threads;

    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            xworker::ExecuteSync(pool_p.get(), [&]() { do_something(counters[cnt]); }, {}, 117);
        }
    });
    descs.push_back("xworker::ExecuteSync(pool) x1");

    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            xworker::ExecuteSync(pool_p.get(), [&]() { do_something(counters[cnt]); }, {}, 120);
        }
    });
    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            xworker::ExecuteSync(pool_p.get(), [&]() { do_something(counters[cnt]); }, {}, 121);
        }
    });

    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            xworker::ExecuteSync(pool_p.get(), [&]() { do_something(counters[cnt]); }, {}, 122);
        }
    });
    descs.push_back("xworker::ExecuteSync(pool) x3");

    uint32_t msec_for_test = 1'000;
    std::this_thread::sleep_for(std::chrono::milliseconds(msec_for_test));

    stop = true;
    std::for_each(threads.begin(), threads.end(), [](auto& th) { th.join(); });

    // xworker::ExecuteSync(
    //     pool_p.get(),
    //     [&]() { do_something(counters[0]); },
    //     {},
    //     121);

    for (size_t z = 0; z < std::min(counters.size(), descs.size()); ++z) {
        std::cout << "ops/msec:" << counters[z].load() / msec_for_test << " " << descs[z] << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_TRUE(pool_p->WorkerState() == xbase::IWorker::State::kStopped);

    pool_p->Join(false);
#ifndef _XSDK_CI_BUILD_
    // EXPECT_FALSE(1);
#endif
}

TEST(xworker_tests, pool_execute_sync_lack_workers)
{
    volatile bool                        stop     = false;
    std::array<std::atomic_uint64_t, 16> counters = {};
    std::vector<std::string>             descs;

    auto pool_p = xworker::CreatePool(0, 2, 300);

    std::atomic_int64_t shared       = {};
    auto                do_something = [&](std::atomic_uint64_t& val) {
        auto v      = val.fetch_add(1);
        auto test_p = std::make_shared<FakeClass>(v);
        auto s      = shared.fetch_add(1);
        test_p->Set(s);
    };

    std::vector<std::thread> threads;

    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            xworker::ExecuteSync(pool_p.get(), [&]() { do_something(counters[cnt]); }, {}, 117);
        }
    });
    descs.push_back("xworker::ExecuteSync(pool) x1");

    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            xworker::ExecuteSync(pool_p.get(), [&]() { do_something(counters[cnt]); }, {}, 120);
        }
    });
    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            xworker::ExecuteSync(pool_p.get(), [&]() { do_something(counters[cnt]); }, {}, 121);
        }
    });

    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            xworker::ExecuteSync(pool_p.get(), [&]() { do_something(counters[cnt]); }, {}, 122);
        }
    });
    descs.push_back("xworker::ExecuteSync(pool) x3");

    uint32_t msec_for_test = 1'000;
    std::this_thread::sleep_for(std::chrono::milliseconds(msec_for_test));

    stop = true;
    std::for_each(threads.begin(), threads.end(), [](auto& th) { th.join(); });

    // xworker::ExecuteSync(
    //     pool_p.get(),
    //     [&]() { do_something(counters[0]); },
    //     {},
    //     121);

    for (size_t z = 0; z < std::min(counters.size(), descs.size()); ++z) {
        std::cout << "ops/msec:" << counters[z].load() / msec_for_test << " " << descs[z] << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(pool_p->WorkerState(), xbase::IWorker::State::kStopped)
        << "WRONG state:" << (uint32_t)pool_p->WorkerState();

    pool_p->Join(false);
#ifndef _XSDK_CI_BUILD_
    // EXPECT_FALSE(1);
#endif
}

TEST(xworker_tests, performanace_compare_sync)
{
    volatile bool                        stop     = false;
    std::array<std::atomic_uint64_t, 16> counters = {};
    std::vector<std::string>             descs;

    auto worker1_p = xworker::CreateWorker();
    auto worker2_p = xworker::CreateWorker();
    auto pool_p    = xworker::CreatePool(1, 8);

    std::atomic_int64_t shared       = {};
    auto                do_something = [&](std::atomic_uint64_t& val) {
        auto v      = val.fetch_add(1);
        auto test_p = std::make_shared<FakeClass>(v);
        auto s      = shared.fetch_add(1);
        test_p->Set(s);
    };

    std::vector<std::thread> threads;
    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop)
            do_something(counters[cnt]);
    });
    descs.push_back("direct");

    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            std::async([&]() { do_something(counters[cnt]); }).wait();
        }
    });
    descs.push_back("std::async(...).wait()");

    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            std::thread([&]() { do_something(counters[cnt]); }).join();
        }
    });
    descs.push_back("std::thread(...).join()");

    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            xworker::ExecuteSync(worker1_p.get(), [&]() { do_something(counters[cnt]); });
        }
    });
    descs.push_back("xworker::ExecuteSync(worker)");

    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            xworker::ExecuteSync(pool_p.get(), [&]() { do_something(counters[cnt]); }, {}, 117);
        }
    });
    descs.push_back("xworker::ExecuteSync(pool)");

    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            xworker::ExecuteSync(worker2_p.get(), [&]() { do_something(counters[cnt]); });
        }
    });
    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            xworker::ExecuteSync(worker2_p.get(), [&]() { do_something(counters[cnt]); });
        }
    });
    descs.push_back("xworker::ExecuteSync(worker) x2");

    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            xworker::ExecuteSync(pool_p.get(), [&]() { do_something(counters[cnt]); }, {}, 120);
        }
    });
    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop) {
            xworker::ExecuteSync(pool_p.get(), [&]() { do_something(counters[cnt]); }, {}, 121);
        }
    });
    descs.push_back("xworker::ExecuteSync(pool) x2");

    uint32_t msec_for_test = 3'000;
    std::this_thread::sleep_for(std::chrono::milliseconds(msec_for_test));

    stop = true;
    std::for_each(threads.begin(), threads.end(), [](auto& th) { th.join(); });

    for (size_t z = 0; z < std::min(counters.size(), descs.size()); ++z) {
        std::cout << "ops/msec:" << counters[z].load() / msec_for_test << " " << descs[z] << std::endl;
    }

#ifndef _XSDK_CI_BUILD_
    // EXPECT_FALSE(1);
#endif
}

TEST(xworker_tests, performanace_compare_async)
{
    volatile bool                        stop     = false;
    std::array<std::atomic_uint64_t, 16> counters = {};
    std::vector<std::string>             descs;

    auto worker1_p = xworker::CreateWorker();
    auto worker2_p = xworker::CreateWorker();
    auto pool_p    = xworker::CreatePool(4, 8);

    std::atomic_int64_t shared       = {};
    auto                do_something = [&](std::atomic_uint64_t& val) {
        val.fetch_add(1);
        shared.fetch_add(1);
    };

    std::vector<std::thread> threads;
    threads.emplace_back([&, cnt = descs.size()]() {
        while (!stop)
            do_something(counters[cnt]);
    });
    descs.push_back("direct");

    std::vector<std::future<xbase::IWorker::FinishType>> tasks_futures;
    tasks_futures.push_back(xworker::ExecuteAsync(worker1_p.get(), [&, cnt = descs.size()]() {
        do_something(counters[cnt]);
        return stop ? xbase::IWorker::RepeatType::kDoNotRepeat : xbase::IWorker::RepeatType::kRepeatUntilCancel;
    }));
    EXPECT_TRUE(tasks_futures.back().valid()) << "xworker::ExecuteAsync(1) FAILED";
    descs.push_back("xworker::ExecuteAsync(worker)");

    tasks_futures.push_back(xworker::ExecuteAsync(pool_p.get(), [&, cnt = descs.size()]() {
        do_something(counters[cnt]);
        return stop ? xbase::IWorker::RepeatType::kDoNotRepeat : xbase::IWorker::RepeatType::kRepeatUntilCancel;
    }));
    EXPECT_TRUE(tasks_futures.back().valid()) << "xworker::ExecuteAsync(1) FAILED";
    descs.push_back("xworker::ExecuteAsync(pool)");

    tasks_futures.push_back(xworker::ExecuteAsync(worker2_p.get(), [&, cnt = descs.size()]() {
        do_something(counters[cnt]);
        return stop ? xbase::IWorker::RepeatType::kDoNotRepeat : xbase::IWorker::RepeatType::kRepeatUntilCancel;
    }));
    EXPECT_TRUE(tasks_futures.back().valid()) << "GlobalPool()->TaskPut(1) FAILED";
    tasks_futures.push_back(xworker::ExecuteAsync(worker2_p.get(), [&, cnt = descs.size()]() {
        do_something(counters[cnt]);
        return stop ? xbase::IWorker::RepeatType::kDoNotRepeat : xbase::IWorker::RepeatType::kRepeatUntilCancel;
    }));
    EXPECT_TRUE(tasks_futures.back().valid()) << "GlobalPool()->TaskPut(2) FAILED";
    descs.push_back("xworker::ExecuteAsync(worker) x2 (switch tasks)");

    tasks_futures.push_back(xworker::ExecuteAsync(pool_p.get(), [&, cnt = descs.size()]() {
        do_something(counters[cnt]);
        return stop ? xbase::IWorker::RepeatType::kDoNotRepeat : xbase::IWorker::RepeatType::kRepeatUntilCancel;
    }));
    EXPECT_TRUE(tasks_futures.back().valid()) << "GlobalPool()->TaskPut(1) FAILED";
    tasks_futures.push_back(xworker::ExecuteAsync(pool_p.get(), [&, cnt = descs.size()]() {
        do_something(counters[cnt]);
        return stop ? xbase::IWorker::RepeatType::kDoNotRepeat : xbase::IWorker::RepeatType::kRepeatUntilCancel;
    }));
    EXPECT_TRUE(tasks_futures.back().valid()) << "GlobalPool()->TaskPut(2) FAILED";
    descs.push_back("xworker::ExecuteAsync(pool) x2");

    uint32_t msec_for_test = 3'000;
    std::this_thread::sleep_for(std::chrono::milliseconds(msec_for_test));

    stop = true;
    for (auto& fut : tasks_futures) {
        if (fut.valid()) {
            auto res = fut.get();
            EXPECT_EQ(res, xbase::IWorker::FinishType::kNormal);
        }
    }

    std::for_each(threads.begin(), threads.end(), [](auto& th) { th.join(); });

    for (size_t z = 0; z < std::min(counters.size(), descs.size()); ++z) {
        std::cout << "ops/msec:" << counters[z].load() / msec_for_test << " " << descs[z] << std::endl;
    }

#ifndef _XSDK_CI_BUILD_
    // EXPECT_FALSE(1);
#endif
}
