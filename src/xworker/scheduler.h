#pragma once

#include "xbase.h"

#include <cassert>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace xsdk::xbase::impl {

class SchedulerImpl final: public IScheduler {

    struct ExecutionData {
        IScheduler::TaskInfo task_info;
        TaskFunction         task_pf;
        IWorker::SPtr        task_worker;
    };

    class SchedulerTask {
        ExecutionData execution_data_;

    public:
        using UPtr = std::unique_ptr<SchedulerTask>;

        SchedulerTask(SchedulerTask&&) = default;
        SchedulerTask(const xbase::Time64                    _scheduled_time,
                      TaskFunction&&                         _task,
                      const std::optional<IWorker::TaskUid>& _task_uid,
                      const IWorker::SPtr&                   _task_worker);

        xbase::Time64        UpdateScheduledTime(const xbase::Time64 _scheduled_time);
        IWorker::TaskUid     TaskId() const { return execution_data_.task_info.task_uid; }
        xbase::Time64        ScheduledTime() const { return execution_data_.task_info.scheduled_time; }
        IScheduler::TaskInfo TaskInfo() const { return execution_data_.task_info; };
        const IWorker::SPtr& Worker() const { return execution_data_.task_worker; }
        ExecutionData        ForExecution(const IClock* _clock_p) const;
        uint64_t             OnExecutionDone() { return ++execution_data_.task_info.scheduling_counter; }
        uint64_t             OnWorkerBusy() { return ++execution_data_.task_info.worker_busy_counter; }
    };

    // 2Thunk: Make class for encapsulate SheduledMap & TimeMap
    using SheduledMap  = std::map<xbase::Time64, SchedulerTask::UPtr>;
    using TimeMap      = std::map<IWorker::TaskUid, xbase::Time64>;
    using ExecutingMap = std::map<IWorker::TaskUid, SchedulerTask::UPtr>;

    mutable std::mutex           mtx_;
    SheduledMap                  tasks_by_time_;
    ExecutingMap                 executing_tasks_;
    TimeMap                      time_by_taskid_;
    std::unique_ptr<std::thread> thread_p_;
    std::condition_variable      wake_up_;

    std::atomic<bool>          stopped_ = {false};
    const xbase::IClock::SPtrC clock_p_;

    const bool          use_static_workers_pool_;
    const IWorker::SPtr default_worker_;

public:
    static constexpr xbase::Time64 kAdvance64           = 500 * time64::kMisec;
    static constexpr xbase::Time64 kRepeatForPoolBusy64 = 10 * time64::kMsec;

    explicit SchedulerImpl(const xbase::IClock* _clock_p,
                           const bool           _use_static_workers_pool,
                           const IWorker::SPtr& _default_worker);
    virtual ~SchedulerImpl() { OnDestroy_(); }

public:
    virtual const xbase::IClock*        Clock() const override { return clock_p_.get(); }
    virtual IWorker::TaskUid            ScheduleTask(const xbase::Time64                    _scheduled_time,
                                                     TaskFunction&&                         _task,
                                                     const std::optional<IWorker::TaskUid>& _task_uid,
                                                     const IWorker::SPtr&                   _task_worker) override;
    virtual std::pair<Status, TaskInfo> TaskStatus(const IWorker::TaskUid _task_uid) const override;
    virtual std::pair<TaskRes, std::future<IWorker::FinishType>> CancelTask(const IWorker::TaskUid _task_uid) override;
    virtual TaskRes RescheduleTask(const IWorker::TaskUid _task_uid, const xbase::Time64 _scheduled_time) override;

private:
    static std::optional<xbase::Time64> Execute_(const ExecutionData& _ed) { return _ed.task_pf(&_ed.task_info); }
    IWorker*                            TaskWorker_(const IWorker::SPtr& _task_worker);

    SheduledMap::node_type ExtractTask_(const IWorker::TaskUid _task_uid);
    IWorker::TaskUid       InsertTask_(const xbase::Time64 _scheduled_time, SchedulerTask::UPtr&& _task_p);
    void                   OnDestroy_();
    xbase::Time64          TillNextRT_(const xbase::Time64 _max_wait) const;
    void                   ThreadRun_();
    bool                   WorkerExecute_(ExecutionData&& _execution_data);
    bool                   ExecutionDone_(const uint64_t                      _task_uid,
                                          bool                                _is_worker_busy,
                                          const std::optional<xbase::Time64> _repeat_time);
};

} // namespace xsdk::xbase::impl
