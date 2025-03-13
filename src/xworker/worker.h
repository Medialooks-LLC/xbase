#pragma once

#include <cassert>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

#include "tasks_queue.h"
#include "xbase.h"

namespace xsdk::xbase::impl {

class WorkerImpl final: public IWorker {

private:
    mutable std::mutex           mtx_;
    std::unique_ptr<std::thread> worker_thread_p_;
    std::unique_ptr<std::thread> expired_thread_p_;
    std::atomic<bool>            joined_ = {false};
    xworker::OnIdleFunction      on_idle_pf_;

    std::atomic<IWorker::TaskUid> executed_task_id_ = {xbase::kInvalidUid};
    std::optional<uint32_t>       idle_timeout_msec_;
    std::optional<size_t>         max_tasks_count_;

    std::condition_variable have_tasks_;
    ITasksQueue::UPtr       tasks_queue_;

public:
    WorkerImpl(xworker::OnIdleFunction&&      _on_idle,
               const std::optional<uint32_t>& _idle_timeout_msec,
               const std::optional<size_t>&   _max_tasks_count);

    virtual ~WorkerImpl();

public:
    virtual TaskUid TaskPut(TaskFunction&&                            _task_pf,
                            std::optional<TaskUid>&&                  _task_uid,
                            std::optional<State>&&                    _required_state_mask,
                            std::optional<std::promise<FinishType>>&& _task_finish_promise) override;

    virtual std::pair<size_t, size_t> TasksCount() const override;

    virtual std::optional<size_t> MaxTasks() const override { return max_tasks_count_; };

    virtual std::thread::id ThreadId() const override;

    virtual std::pair<CancelRes, std::future<FinishType>> TaskCancel(const TaskUid _task_uid) override;

    virtual size_t TaskCancelAll() override;

    virtual bool Join(const bool _cancel_tasks) override;

    virtual State WorkerState() const override;

private:
    size_t TasksTotal_() const;

    bool WaitForTasks_(std::unique_lock<std::mutex>& _lck, std::optional<uint32_t> _max_wait_msec = {});

    bool JoinExpired_();

    void ThreadRun_();
};
} // namespace xsdk::xbase::impl
