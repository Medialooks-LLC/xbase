#pragma once

#include <cassert>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <thread>

#include "worker.h" // Temp for WorkerBase
#include "xbase.h"

namespace xsdk::xbase::impl {

class UsageCounter {
public:
    using Token = std::shared_ptr<std::monostate>;

    UsageCounter() : token_sp_(std::make_shared<std::monostate>()) {}

    Token  GetToken() { return token_sp_; }
    size_t TokensCounter() const { return token_sp_.use_count() - 1; }

private:
    Token token_sp_;
};

class PoolWorkersImpl: public IWorker {

    static constexpr xbase::Time64 kNonActivePrefersTimeout = time64::FromSec(60);

    mutable std::shared_mutex rw_;

    mutable std::vector<IWorker::UPtr> workers_;
    std::atomic<size_t>                executing_now_ = {};
    const size_t                       min_workers_   = 0;
    const size_t                       max_workers_   = 0;
    const uint32_t                     idle_timeout_  = 0;
    std::optional<size_t>              max_tasks_count_;
    const bool                         fast_on_idle_ = true;

    class PreferWorker {
        size_t          worker_idx_         = xbase::npos;
        xbase::Time64   activity_timestamp_ = time64::kNoVal;
        std::thread::id worker_thread_id_;

    public:
        // For debug/perfomance check
        size_t succeeded = 0;
        size_t failed    = 0;

    public:
        PreferWorker() : activity_timestamp_(xclock::UtcTime()) {}
        //~PreferWorker() { std::cout << (double)succeeded / (succeeded + failed) << std::endl; }
        void          SetPreferedWorker(const size_t _idx, const std::thread::id _worker_thread_id);
        IWorker*      GetPreferedWorker(const std::vector<IWorker::UPtr>& _workers);
        void          UpdateActivity() { activity_timestamp_ = xclock::UtcTime(); }
        xbase::Time64 InactiveTime() const { return xclock::UtcTime() - activity_timestamp_; }
    };
    std::map<IWorker::TaskUid, PreferWorker> tasks_prefer_workers_;

    // Awaited tasks
    ITasksQueue::UPtr tasks_queue_;

    xbase::ClockHR                 clock_expired_;
    static constexpr xbase::Time64 kCheckExpiredPeriod64 = time64::FromMsec(100);

public:
    PoolWorkersImpl(size_t                       _min_workers,
                    size_t                       _max_workers,
                    const uint32_t               _idle_timeout,
                    const std::optional<size_t>& _max_tasks_count,
                    bool                         _fast_on_idle);

    virtual ~PoolWorkersImpl() { PoolWorkersImpl::Join(true); }

public:
    virtual IWorker::TaskUid TaskPut(TaskFunction&&                            _task_pf,
                                     std::optional<TaskUid>&&                  _task_uid,
                                     std::optional<State>&&                    _required_state_mask,
                                     std::optional<std::promise<FinishType>>&& _task_finish_promise) override;

    virtual std::pair<size_t, size_t> TasksCount() const override;

    virtual std::optional<size_t> MaxTasks() const override { return max_tasks_count_; };

    virtual std::thread::id ThreadId() const override { return {}; }

    virtual std::pair<CancelRes, std::future<FinishType>> TaskCancel(const TaskUid _task_uid) override;

    virtual bool Join(const bool _cancel_tasks) override;

    virtual State WorkerState() const override;

private:
    State ThreadState_() const;

    size_t FreeExpiredWorkers_() const;

    size_t FreeExpiredPrefers_(const xbase::Time64 _non_active_timeout);

    //std::optional<size_t> WorkerIndex_(const IWorker* _idle_worker_p);

    void OnIdle_(IWorker* _idle_worker_p);

    std::pair<size_t, IWorker*> AddWorker_(const std::optional<uint32_t>& _idle_timeout_msec);

    IWorker::TaskUid ExecuteTask_(TaskFunction&&                            _task_pf,
                                  std::optional<TaskUid>&&                  _task_uid,
                                  std::optional<std::promise<FinishType>>&& _task_finish_promise);
};

} // namespace xsdk::xbase::impl
