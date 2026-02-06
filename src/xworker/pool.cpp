#include "pool.h"

namespace xsdk {

xbase::IWorker::UPtr xworker::CreatePool(const size_t                 _min_workers,
                                         const size_t                 _max_workers,
                                         const uint32_t               _idle_timeout_msec,
                                         const std::optional<size_t> _max_tasks_count,
                                         OnThreadStartedFunction&&    _on_started,
                                         OnThreadFinishedFunction&&   _on_finished)
{
    return std::make_unique<xbase::impl::PoolWorkersImpl>(_min_workers,
                                                          _max_workers,
                                                          _idle_timeout_msec,
                                                          _max_tasks_count,
                                                          std::move(_on_started),
                                                          std::move(_on_finished),
                                                          true);
}

xbase::IWorker* xworker::StaticPool()
{
    static xbase::IWorker::UPtr pool = xworker::CreatePool(xworker::kWorkersPoolMinSize,
                                                           xworker::kWorkersPoolMaxSize,
                                                           xworker::kWorkersPoolIdleTimeoutMsec);
    return pool.get();
}

namespace xbase::impl {

    void PoolWorkersImpl::PreferWorker::SetPreferedWorker(const size_t _idx, const std::thread::id _worker_thread_id)
    {
        worker_idx_         = _idx;
        worker_thread_id_   = _worker_thread_id;
        activity_timestamp_ = xclock::UtcTime();
    }
    IWorker* PoolWorkersImpl::PreferWorker::GetPreferedWorker(const std::vector<IWorker::UPtr>& _workers)
    {
        // Check for recent worker index
        if (worker_idx_ < _workers.size() && _workers[worker_idx_]->ThreadId() == worker_thread_id_)
            return _workers[worker_idx_].get();

        // Try find worker with required thread_id
        for (size_t idx = 0; idx < _workers.size(); ++idx) {
            if (_workers[idx]->ThreadId() == worker_thread_id_) {
                worker_idx_ = idx;
                return _workers[worker_idx_].get();
            }
        }

        // Preferred worker not found, reset activity time for fast remove at next FreeExpiredPrefers_
        activity_timestamp_ = xclock::UtcTime() - time64::kDay * 365;
        return nullptr;
    }

    PoolWorkersImpl::PoolWorkersImpl(size_t                              _min_workers,
                                     size_t                              _max_workers,
                                     const uint32_t                      _idle_timeout,
                                     const std::optional<size_t>        _max_tasks_count,
                                     xworker::OnThreadStartedFunction&&  _on_started,
                                     xworker::OnThreadFinishedFunction&& _on_finished,
                                     bool                                _fast_on_idle)
        : min_workers_(_min_workers),
          max_workers_(std::max<size_t>(1,_max_workers)),
          idle_timeout_(_idle_timeout),
          max_tasks_count_(_max_tasks_count),
          on_started_pf_(std::move(_on_started)),
          on_finished_pf_(std::move(_on_finished)),
          fast_on_idle_(_fast_on_idle),
          tasks_queue_(CreateTaskQueue())
    {
        assert(_max_workers > 0);
        // Add minimum workers w/o timeout -> always running - no expiration
        for (size_t z = 0; z < _min_workers; ++z)
            AddWorker_(std::nullopt);
    }

    IWorker::TaskUid PoolWorkersImpl::TaskPut(TaskFunction&&                            _task_pf,
                                              const std::optional<TaskUid>                  _task_uid,
                                              const std::optional<State>                    _required_state_mask,
                                              std::optional<std::promise<FinishType>>&& _task_finish_promise)
    {
        assert(_task_uid.value_or(~xbase::kInvalidUid) != xbase::kInvalidUid);

        std::shared_lock lck_r(rw_);

        if (executing_now_.load() < workers_.size()) {
            // Try execute task on current workers
            auto task_uid = ExecuteTask_(std::move(_task_pf), std::move(_task_uid), std::move(_task_finish_promise));
            if (task_uid != xbase::kInvalidUid)
                return task_uid;
        }

        lck_r.unlock();

        std::unique_lock lck_w(rw_);

        if (_required_state_mask.has_value() && !(ThreadState_() & _required_state_mask.value()))
            return xbase::kInvalidUid;

        auto [idx, worker_p] = AddWorker_(idle_timeout_);
        if (worker_p) {
            auto task_uid = _task_uid.value_or(xbase::kInvalidUid);

            executing_now_.fetch_add(1);
            auto added = worker_p->TaskPut(std::move(_task_pf),
                                           std::move(_task_uid),
                                           {},
                                           std::move(_task_finish_promise));
            assert(added != xbase::kInvalidUid);
            assert(task_uid == xbase::kInvalidUid || task_uid == added);
            if (task_uid != xbase::kInvalidUid)
                tasks_prefer_workers_[added].SetPreferedWorker(idx, worker_p->ThreadId());

            return added;
        }

        if (tasks_queue_->Size() >= max_tasks_count_.value_or(xbase::npos))
            return xbase::kInvalidUid;

        ITasksQueue::Task task = {_task_uid.value_or(xbase::kInvalidUid),
                                  std::move(_task_pf),
                                  std::move(_task_finish_promise)};

        auto task_uid = tasks_queue_->EmplaceBack(true, std::move(task));
        assert(task_uid != xbase::kInvalidUid);

        if (fast_on_idle_ && executing_now_.load() < workers_.size()) {
            // Worker finish right before task added to deque, see OnIdle_ (*)
            auto task_next = tasks_queue_->TakeFront();
            assert(task_next.has_value());

            auto executed = ExecuteTask_(std::move(task_next->task_pf),
                                         task_next->task_uid,
                                         std::move(task_next->finish_promise));
            assert(executed == xbase::kInvalidUid || executed == task_next->task_uid);
            if (executed == xbase::kInvalidUid) { // Looks like this is dead code
                // Workers could be already busy, return task to deque
                [[maybe_unused]] auto added = tasks_queue_->EmplaceFront(true, std::move(task_next.value()));
                assert(added != xbase::kInvalidUid);
            }
        }

        return task_uid;
    }

   IWorker::Status PoolWorkersImpl::WorkerStatus() const
    {
       const std::shared_lock lck(rw_);

       return {executing_now_.load(), tasks_queue_->Size(), ThreadState_()};
    }

    std::pair<IWorker::CancelRes, std::future<IWorker::FinishType>> PoolWorkersImpl::TaskCancel(const TaskUid _task_uid)
    {
        if (_task_uid == xbase::kInvalidUid)
            return {CancelRes::kInvalidUid, std::future<IWorker::FinishType> {}};

        const std::unique_lock lck(rw_);

        if (tasks_queue_->Erase(_task_uid, FinishType::kCanceled))
            return {CancelRes::kCanceled, std::future<IWorker::FinishType> {}};

        for (const auto& worker_p : workers_) {
            auto [res, cancel_future] = worker_p->TaskCancel(_task_uid);
            if (res != CancelRes::kNotFound)
                return {res, std::move(cancel_future)};
        }

        return {CancelRes::kNotFound, std::future<IWorker::FinishType> {}};
    }

    size_t PoolWorkersImpl::TaskCancelAll()
    {
        std::unique_lock lck(rw_);

        size_t canceled = tasks_queue_->EraseAll(FinishType::kCanceled);

        auto workers = std::exchange(workers_, std::vector<IWorker::UPtr>());
        tasks_prefer_workers_.clear();
        lck.unlock();

        for (auto& worker_p : workers)
            canceled += worker_p->TaskCancelAll();

        for (size_t z = 0; z < min_workers_; ++z)
            AddWorker_(std::nullopt);

        return canceled;
    }

    bool PoolWorkersImpl::Join(const bool _cancel_tasks)
    {
        std::unique_lock lck(rw_);

        if (_cancel_tasks)
            tasks_queue_->EraseAll(FinishType::kCanceled);

        auto workers = std::exchange(workers_, std::vector<IWorker::UPtr>());
        tasks_prefer_workers_.clear();
        lck.unlock();

        for (auto& worker_p : workers)
            worker_p->Join(_cancel_tasks);

        return true;
    }

    IWorker::State PoolWorkersImpl::WorkerState() const
    {
        const std::unique_lock lck(rw_);

        FreeExpiredWorkers_();

        return ThreadState_();
    }

    IWorker::State PoolWorkersImpl::ThreadState_() const
    {
        if (!tasks_queue_->Empty() || executing_now_.load() >= max_workers_)
            return State::kBusy;

        return workers_.empty() ? State::kStopped : State::kIdle;
    }

    size_t PoolWorkersImpl::FreeExpiredWorkers_() const
    {
        size_t removed = 0;

        auto it = workers_.rbegin();
        while (it != workers_.rend() && workers_.size() > executing_now_.load() + min_workers_) {
            if ((*it)->WorkerState() == State::kStopped) {
                it = decltype(it)(workers_.erase(std::next(it).base()));
                ++removed;
                continue;
            }

            ++it;
        }

        return removed;
    }
    size_t PoolWorkersImpl::FreeExpiredPrefers_(const xbase::Time64 _non_active_timeout)
    {
        size_t expired = 0;

        auto it = tasks_prefer_workers_.begin();
        while (it != tasks_prefer_workers_.end()) {
            if (it->second.InactiveTime() > _non_active_timeout || !it->second.GetPreferedWorker(workers_)) {
                it = tasks_prefer_workers_.erase(it);
                ++expired;
                continue;
            }

            ++it;
        }

        return expired;
    }
    // std::optional<size_t> PoolWorkersImpl::WorkerIndex_(const IWorker* _idle_worker_p)
    //{
    //     for (size_t idx = 0; idx < workers_.size(); ++idx)
    //         if (workers_[idx].get() == _idle_worker_p)
    //             return idx;

    //    return std::nullopt;
    //}
    void PoolWorkersImpl::OnIdle_(IWorker* _idle_worker_p)
    {
        // Check first w/o lock for better performance
        // Theretically right after next check, task could be added into queue and await free worker
        // but this is handled in TaskPut() latest block (*)
        if (fast_on_idle_ && tasks_queue_->Empty()) {
            assert(executing_now_.load() > 0);
            executing_now_.fetch_sub(1);

            if (clock_expired_.ResetLap(kCheckExpiredPeriod64).first) {
                const std::unique_lock lck(rw_);
                FreeExpiredWorkers_();
                FreeExpiredPrefers_(kNonActivePrefersTimeout);
            }

            return;
        }

        const std::unique_lock lck(rw_);

        if (clock_expired_.ResetLap(kCheckExpiredPeriod64).first) {
            FreeExpiredWorkers_();
            FreeExpiredPrefers_(kNonActivePrefersTimeout);
        }

        if (tasks_queue_->Empty()) {
            assert(executing_now_.load() > 0);
            executing_now_.fetch_sub(1);
            return;
        }

        auto task = tasks_queue_->TakeFront();
        assert(task.has_value());
        if (task.has_value()) {
            auto added = _idle_worker_p->TaskPut(std::move(task->task_pf),
                                                 task->task_uid,
                                                 IWorker::State::kIdle,
                                                 std::move(task->finish_promise));
            if (added == xbase::kInvalidUid) {
                // Worker could be already busy, return task to deque
                tasks_queue_->EmplaceFront(true, std::move(task.value()));
                executing_now_.fetch_sub(1);
            }
        }

        // auto idx = WorkerIndex_(_idle_worker_p);
        // if (idx.has_value())
        //     tasks_prefer_workers_[task.task_uid].SetPreferedWorker(idx.value(), workers_[idx.value()]->ThreadId());
    }

    std::pair<size_t, IWorker*> PoolWorkersImpl::AddWorker_(const std::optional<uint32_t> _idle_timeout_msec)
    {
        if (workers_.size() >= max_workers_)
            return {xbase::npos, nullptr};

        auto worker_p = xworker::CreateWorker([this](auto* _worker_p) { OnIdle_(_worker_p); },
                                              _idle_timeout_msec,
                                              {}, // 2Think: to add max task ?
                                              xworker::OnThreadStartedFunction(on_started_pf_),
                                              xworker::OnThreadFinishedFunction(on_finished_pf_));

        workers_.push_back(std::move(worker_p));
        return {workers_.size() - 1, workers_.back().get()};
    }

    IWorker::TaskUid PoolWorkersImpl::ExecuteTask_(TaskFunction&&                            _task_pf,
                                                   const std::optional<TaskUid>                  _task_uid,
                                                   std::optional<std::promise<FinishType>>&& _task_finish_promise)
    {
        auto task_uid = _task_uid.value_or(xbase::kInvalidUid);

        // Try prefered (recent) worker
        if (task_uid != xbase::kInvalidUid) {
            auto it = tasks_prefer_workers_.find(_task_uid.value());
            if (it != tasks_prefer_workers_.end()) {
                auto* worker_p = it->second.GetPreferedWorker(workers_);
                if (worker_p) {
                    it->second.UpdateActivity();
                    executing_now_.fetch_add(1);
                    auto added = worker_p->TaskPut(std::move(_task_pf),
                                                   std::move(_task_uid),
                                                   IWorker::State::kIdle,
                                                   std::move(_task_finish_promise));
                    if (added != xbase::kInvalidUid) {
                        ++it->second.succeeded;
                        return added;
                    }
                    executing_now_.fetch_sub(1);
                }

                ++it->second.failed;
            }
        }

        std::vector<std::pair<size_t, IWorker*>> closed_workers;

        for (size_t idx = 0; idx < workers_.size(); ++idx) {
            const auto& worker_p = workers_[idx];

            executing_now_.fetch_add(1);
            auto added = worker_p->TaskPut(std::move(_task_pf),
                                           std::move(_task_uid),
                                           IWorker::State::kIdle,
                                           std::move(_task_finish_promise));
            if (added != xbase::kInvalidUid) {
                assert(task_uid == xbase::kInvalidUid || task_uid == added);
                if (task_uid != xbase::kInvalidUid)
                    tasks_prefer_workers_[added].SetPreferedWorker(idx, worker_p->ThreadId());

                return added;
            }
            executing_now_.fetch_sub(1);

            // If worker is closed -> try in next cicle
            if (worker_p->WorkerState() == IWorker::State::kStopped) {
                closed_workers.emplace_back(idx, worker_p.get());
                continue;
            }
        }

        // Try from closed workers
        for (const auto [idx, worker_p] : closed_workers) {
            executing_now_.fetch_add(1);
            auto added = worker_p->TaskPut(std::move(_task_pf),
                                           std::move(_task_uid),
                                           IWorker::State::kIdle | IWorker ::State::kStopped,
                                           std::move(_task_finish_promise));
            if (added != xbase::kInvalidUid) {
                assert(task_uid == xbase::kInvalidUid || task_uid == added);
                if (task_uid != xbase::kInvalidUid)
                    tasks_prefer_workers_[added].SetPreferedWorker(idx, worker_p->ThreadId());

                return added;
            }

            executing_now_.fetch_sub(1);
        }

        // No free workers
        return xbase::kInvalidUid;
    }

} // namespace xbase::impl
} // namespace xsdk