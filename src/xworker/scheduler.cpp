#include "scheduler.h"

namespace xsdk {

xbase::IScheduler::UPtr xscheduler::CreateScheduler(const xbase::IClock* _clock_p,
                                                    const bool           _use_static_workers_pool,
                                                    const IWorker::SPtr& _default_worker)
{
    return std::make_unique<xbase::impl::SchedulerImpl>(_clock_p, _use_static_workers_pool, _default_worker);
}

xbase::IScheduler* xscheduler::StaticScheduler()
{
    static xbase::IScheduler::UPtr scheduler = xscheduler::CreateScheduler(xclock::UtcClock(true), true);
    return scheduler.get();
}

namespace xbase::impl {
    SchedulerImpl::SchedulerTask::SchedulerTask(const xbase::Time64                    _scheduled_time,
                                                TaskFunction&&                         _task,
                                                const std::optional<IWorker::TaskUid>& _task_uid,
                                                const IWorker::SPtr&                   _task_worker)
    {
        execution_data_.task_worker              = _task_worker;
        execution_data_.task_pf                  = std::move(_task);
        execution_data_.task_info.task_uid       = _task_uid.value_or(xbase::kInvalidUid) != xbase::kInvalidUid ?
                                                       _task_uid.value() :
                                                       xbase::NextUid();
        execution_data_.task_info.scheduled_time = _scheduled_time;
    }
    xbase::Time64 SchedulerImpl::SchedulerTask::UpdateScheduledTime(const xbase::Time64 _scheduled_time)
    {
        return std::exchange(execution_data_.task_info.scheduled_time, _scheduled_time);
    }
    SchedulerImpl::ExecutionData SchedulerImpl::SchedulerTask::ForExecution(const IClock* _clock_p) const
    {
        ExecutionData execution_data     = execution_data_;
        execution_data.task_info.clock_p = _clock_p;
        return execution_data;
    }

    SchedulerImpl::SchedulerImpl(const xbase::IClock* _clock_p,
                                 const bool           _use_static_workers_pool,
                                 const IWorker::SPtr& _default_worker)
        : clock_p_(xclock::CreateOrClone(_clock_p, xclock::SteadySyncGen(true))),
          use_static_workers_pool_(_use_static_workers_pool),
          default_worker_(_use_static_workers_pool ? nullptr : _default_worker)
    {
        assert(!(_use_static_workers_pool && _default_worker));
        // For scheduler is highly recommented to use monotonic sync gen
        assert(clock_p_ && clock_p_->SyncGenerator()->IsMonotonicIncrease());
    }
    IWorker::TaskUid SchedulerImpl::ScheduleTask(const xbase::Time64                    _scheduled_time,
                                                 TaskFunction&&                         _task,
                                                 const std::optional<IWorker::TaskUid>& _task_uid,
                                                 const IWorker::SPtr&                   _task_worker)
    {
        assert(_task);
        if (!_task)
            return xbase::kInvalidUid;

        assert(_task_uid.value_or(~xbase::kInvalidUid) != xbase::kInvalidUid);

        const std::unique_lock lck(mtx_);

        if (!thread_p_)
            thread_p_ = std::make_unique<std::thread>([this]() { ThreadRun_(); });

        // optimization: execute task right now if time less than current
        auto corrected_time = std::max(_scheduled_time, clock_p_->Time());
        return InsertTask_(corrected_time,
                           std::make_unique<SchedulerTask>(corrected_time, std::move(_task), _task_uid, _task_worker));
    }

    std::pair<IScheduler::Status, IScheduler::TaskInfo> SchedulerImpl::TaskStatus(
        const IWorker::TaskUid _task_uid) const
    {
        if (_task_uid == xbase::kInvalidUid)
            return {Status::kInvalidUid, {}};

        const std::unique_lock lck(mtx_);

        // Check executing now
        auto it_exe = executing_tasks_.find(_task_uid);
        if (it_exe != executing_tasks_.end())
            return {Status::kExecutingNow, it_exe->second->TaskInfo()};

        auto it = time_by_taskid_.find(_task_uid);
        if (it == time_by_taskid_.end())
            return {Status::kNotFound, {}};

        auto it_task = tasks_by_time_.find(it->second);
        assert(it_task != tasks_by_time_.end());
        return {Status::kScheduled, it_task->second->TaskInfo()};
    }

    std::pair<IScheduler::TaskRes, std::future<IWorker::FinishType>> SchedulerImpl::CancelTask(
        const IWorker::TaskUid _task_uid)
    {
        if (_task_uid == xbase::kInvalidUid)
            return {TaskRes::kInvalidUid, std::future<IWorker::FinishType> {}};

        std::unique_lock lck(mtx_);

        auto nh = ExtractTask_(_task_uid);
        if (!nh.empty())
            return {TaskRes::kOk, std::future<IWorker::FinishType> {}};

        auto it = executing_tasks_.find(_task_uid);
        if (it == executing_tasks_.end())
            return {TaskRes::kNotFound, std::future<IWorker::FinishType> {}};

        IWorker::SPtr task_worker = it->second->Worker();
        executing_tasks_.erase(it);

        // unlock mutex as worker may be locked WorkerExecute_ -> labmda
        lck.unlock();

        auto* worker_p = TaskWorker_(task_worker);
        if (worker_p) {
            auto [res, cancel_future] = worker_p->TaskCancel(_task_uid);
            if (res == IWorker::CancelRes::kExecutingNow)
                return {TaskRes::kExecutingNow, std::move(cancel_future)};
        }

        // WARNING !!! For schedulers w/o workers -> cancel future not implemented
        return {TaskRes::kOk, std::future<IWorker::FinishType> {}};
    }

    IScheduler::TaskRes SchedulerImpl::RescheduleTask(const IWorker::TaskUid _task_uid,
                                                      const xbase::Time64    _scheduled_time)
    {
        if (_task_uid == xbase::kInvalidUid)
            return TaskRes::kInvalidUid;

        std::unique_lock lck(mtx_);

        auto nh = ExtractTask_(_task_uid);
        if (nh.empty())
            return executing_tasks_.count(_task_uid) > 0 ? TaskRes::kExecutingNow : TaskRes::kNotFound;

        if (TaskWorker_(nh.mapped()->Worker()) && _scheduled_time + kAdvance64 < clock_p_->Time()) {

            auto [it, is_inserted] = executing_tasks_.try_emplace(_task_uid, std::move(nh.mapped()));
            assert(is_inserted);
            auto execution_data = it->second->ForExecution(clock_p_.get());
            lck.unlock();

            const auto task_uid = execution_data.task_info.task_uid;
            if (!WorkerExecute_(std::move(execution_data))) {
                lck.lock();
                ExecutionDone_(task_uid, true, clock_p_->Time() + kRepeatForPoolBusy64);
                return TaskRes::kWorkerBusy;
            }

            return TaskRes::kForcedNow;
        }

        auto min_time = clock_p_->Time();
        if (!tasks_by_time_.empty())
            min_time = std::min(min_time, tasks_by_time_.begin()->first - 1);

        auto corrected_time = std::max(min_time, _scheduled_time);
        InsertTask_(corrected_time, std::move(nh.mapped()));
        return TaskRes::kOk;
    }

    IWorker* SchedulerImpl::TaskWorker_(const IWorker::SPtr& _task_worker)
    {
        if (_task_worker)
            return _task_worker.get();

        if (use_static_workers_pool_)
            return xworker::StaticPool();

        return default_worker_.get();
    }

    SchedulerImpl::SheduledMap::node_type SchedulerImpl::ExtractTask_(const IWorker::TaskUid _task_uid)
    {
        auto nh_time = time_by_taskid_.extract(_task_uid);
        if (nh_time.empty())
            return {};

        return tasks_by_time_.extract(nh_time.mapped());
    }

    IWorker::TaskUid SchedulerImpl::InsertTask_(const xbase::Time64 _scheduled_time, SchedulerTask::UPtr&& _task_p)
    {
        // Task already exist
        if (_task_p->TaskId() != xbase::kInvalidUid && time_by_taskid_.count(_task_p->TaskId()) > 0)
            return xbase::kInvalidUid;

        auto time_as_key = _scheduled_time;
        while (true) {
            auto [it, is_emplaced] = tasks_by_time_.try_emplace(time_as_key, std::move(_task_p));
            if (!is_emplaced) {
                ++time_as_key;
                continue;
            }

            it->second->UpdateScheduledTime(_scheduled_time);
            time_by_taskid_[it->second->TaskId()] = it->first;
            assert(time_by_taskid_.size() == tasks_by_time_.size());

            if (it == tasks_by_time_.begin())
                wake_up_.notify_all();

            return it->second->TaskId();
        }
    }

    void SchedulerImpl::OnDestroy_()
    {
        std::unique_lock lck(mtx_);
        stopped_.store(true);
        wake_up_.notify_all();
        lck.unlock();

        if (thread_p_ && thread_p_->joinable()) {
            assert(thread_p_->get_id() != std::this_thread::get_id());
            thread_p_->join();
            // if (thread_p_->get_id() != std::this_thread::get_id())
            //     thread_p_->join();
            // else
            //     thread_p_->detach();
        }
    }

    xbase::Time64 SchedulerImpl::TillNextRT_(const xbase::Time64 _max_wait) const
    {
        return tasks_by_time_.empty() ?
                   _max_wait :
                   std::min<xbase::Time64>(_max_wait,
                                           tasks_by_time_.begin()->second->ScheduledTime() - clock_p_->Time());
    }

    void SchedulerImpl::ThreadRun_()
    {
        std::unique_lock lck(mtx_);

        TaskInfo info;
        info.clock_p = clock_p_.get();
        while (!stopped_.load()) {
            auto wait_rt = TillNextRT_(time64::kMinute);
            if (wait_rt > kAdvance64) {
                wake_up_.wait_for(lck, std::chrono::microseconds(wait_rt / time64::kMisec));
                continue;
            }

            // Move to executing map
            assert(!tasks_by_time_.empty());
            auto nh = tasks_by_time_.extract(tasks_by_time_.begin());
            time_by_taskid_.erase(nh.mapped()->TaskId());
            auto [it, is_inserted] = executing_tasks_.try_emplace(nh.mapped()->TaskId(), std::move(nh.mapped()));
            assert(is_inserted);
            auto execution_data = it->second->ForExecution(clock_p_.get());
            lck.unlock();

            const auto task_uid = execution_data.task_info.task_uid;
            if (TaskWorker_(execution_data.task_worker)) {
                bool is_busy = !WorkerExecute_(std::move(execution_data));
                lck.lock();
                if (is_busy)
                    ExecutionDone_(task_uid, true, clock_p_->Time() + kRepeatForPoolBusy64);
            }
            else {
                auto repeat_rt = Execute_(execution_data);
                lck.lock();
                ExecutionDone_(task_uid, false, repeat_rt);
            }
        }
    }

    bool SchedulerImpl::WorkerExecute_(ExecutionData&& _execution_data)
    {
        const auto task_uid = _execution_data.task_info.task_uid;
        assert(task_uid != xbase::kInvalidUid);

        // For do not pass worker into lambda !!!
        auto  task_worker = std::exchange(_execution_data.task_worker, xbase::IWorker::SPtr());
        auto* worker_p    = TaskWorker_(task_worker);
        assert(worker_p);
        auto execution_data_sp = xbase::ToShared(std::move(_execution_data));
        auto check_task_uid = worker_p->TaskPut(
            [this, execution_data_sp]() mutable {
                auto repeat_rt = Execute_(*execution_data_sp);
                if (repeat_rt.has_value() && repeat_rt.value() <= clock_p_->Time() + kAdvance64) {
                    execution_data_sp->task_info.scheduled_time = repeat_rt.value();
                    ++execution_data_sp->task_info.scheduling_counter;
                    return IWorker::RepeatType::kRepeatUntilCancel;
                }

                const std::unique_lock lck(mtx_);
                ExecutionDone_(execution_data_sp->task_info.task_uid, false, repeat_rt);
                return IWorker::RepeatType::kDoNotRepeat;
            },
            task_uid);

        // TaskPut could be failed if worker have tasks limit
        assert(check_task_uid == xbase::kInvalidUid || check_task_uid == task_uid);
        return check_task_uid != xbase::kInvalidUid;
    }

    bool SchedulerImpl::ExecutionDone_(const uint64_t                      _task_uid,
                                       bool                                _is_worker_busy,
                                       const std::optional<xbase::Time64> _repeat_time)
    {
        // Task could be removed from executing_tasks if it's canceled
        auto nh = executing_tasks_.extract(_task_uid);
        if (!nh || !_repeat_time.has_value())
            return false;

        if (_is_worker_busy)
            nh.mapped()->OnWorkerBusy();
        else
            nh.mapped()->OnExecutionDone();

        auto corrected_time = std::max(clock_p_->Time(), _repeat_time.value());
        InsertTask_(corrected_time, std::move(nh.mapped()));
        return true;
    }

} // namespace xbase::impl
} // namespace xsdk