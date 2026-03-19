#pragma once

#include "preferred_workers.h"

// #include "map_thread_safe.hpp"
#include "worker.h" // Temp for WorkerBase
#include "xbase.h"

#include <cassert>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <thread>

namespace xsdk::xbase::impl {

// 2Think: Move to common helpers
namespace counter {
    template <typename TNumber>
    using AtomicSPtr = std::shared_ptr<std::atomic<TNumber>>;

    template <typename TNumber>
    AtomicSPtr<TNumber> CreateCounter(const TNumber _initial = {})
    {
        return std::make_shared<std::atomic<TNumber>>(_initial);
    }

    template <typename TNumber>
    class Holder {
        AtomicSPtr<TNumber> counter_sp_;

    public:
        using SPtr = std::shared_ptr<Holder<TNumber>>;

        Holder(const AtomicSPtr<TNumber>& _counter_sp) : counter_sp_(_counter_sp)
        {
            assert(counter_sp_);
            counter_sp_->fetch_add(1);
        }

        Holder(Holder&& _move) noexcept : counter_sp_(std::exchange(_move.counter_sp_, {})) {}

        Holder(const Holder& _move) = delete;

        ~Holder()
        {
            if (counter_sp_)
                counter_sp_->fetch_sub(1);
        }
    };

    template <typename TNumber>
    std::shared_ptr<Holder<TNumber>> CreateHolder(const AtomicSPtr<TNumber>& _counter_sp)
    {
        return std::make_shared<Holder<TNumber>>(_counter_sp);
    }

} // namespace counter

class PoolWorkersImpl: public IWorker {

    static constexpr xbase::Time64 kNonActivePreferenceTimeout = time64::FromSec(60);
    static constexpr xbase::Time64 kCheckExpiredPeriod64       = time64::FromMsec(100);

    const size_t                            min_workers_  = 0;
    const size_t                            max_workers_  = 0;
    const uint32_t                          idle_timeout_ = 0;
    const std::optional<size_t>             max_tasks_count_;
    const xworker::OnThreadStartedFunction  on_started_pf_;
    const xworker::OnThreadFinishedFunction on_finished_pf_;
    const bool                              fast_on_idle_ = true;

    // Awaited tasks
    const ITasksQueue::UPtr tasks_queue_;

    const xbase::IClock::UPtr clock_expired_;

    const counter::AtomicSPtr<size_t> executing_now_p_ = counter::CreateCounter<size_t>(0);

    PreferredWorkers preferred_workers_; // Thread-safe

    mutable std::shared_mutex          rw_;
    mutable std::vector<IWorker::UPtr> workers_; // mutable for const FreeExpiredWorkers_ (TODO: Change logic)

public:
    PoolWorkersImpl(size_t                              _min_workers,
                    size_t                              _max_workers,
                    const uint32_t                      _idle_timeout,
                    const std::optional<size_t>         _max_tasks_count,
                    xworker::OnThreadStartedFunction&&  _on_started,
                    xworker::OnThreadFinishedFunction&& _on_finished,
                    bool                                _fast_on_idle);

    virtual ~PoolWorkersImpl() { PoolWorkersImpl::Join(true); }

public:
    virtual IWorker::TaskUid TaskPut(TaskFunction&&                            _task_pf,
                                     const std::optional<TaskUid>              _task_uid,
                                     const std::optional<State>                _required_state_mask,
                                     std::optional<std::promise<FinishType>>&& _task_finish_promise) override;

    virtual IWorker::Status WorkerStatus() const override;

    virtual std::optional<size_t> MaxTasks() const override { return max_tasks_count_; };

    virtual std::thread::id ThreadId() const override { return {}; }

    virtual std::pair<CancelRes, std::future<FinishType>> TaskCancel(const TaskUid _task_uid) override;

    virtual size_t TaskCancelAll() override;

    virtual bool Join(const bool _cancel_tasks) override;

    virtual State WorkerState() const override;

private:
    IWorker::TaskFunction TaskWithCounter_(const IWorker::TaskFunction& _task_pf) const;

    State ThreadState_() const;

    size_t FreeExpiredWorkers_() const;

    void OnIdle_(IWorker* _idle_worker_p);

    std::pair<size_t, IWorker*> AddWorker_(const std::optional<uint32_t> _idle_timeout_msec);

    IWorker::TaskUid ExecuteTask_(TaskFunction&&                            _task_pf,
                                  const std::optional<TaskUid>              _task_uid,
                                  std::optional<std::promise<FinishType>>&& _task_finish_promise);
};

} // namespace xsdk::xbase::impl
