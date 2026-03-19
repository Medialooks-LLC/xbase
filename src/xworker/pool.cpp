#include "pool.h"
#include "xbase/xuid.h"

namespace xsdk {

xbase::IWorker::UPtr xworker::CreatePool(const size_t                _min_workers,
                                         const size_t                _max_workers,
                                         const uint32_t              _idle_timeout_msec,
                                         const std::optional<size_t> _max_tasks_count,
                                         OnThreadStartedFunction&&   _on_started,
                                         OnThreadFinishedFunction&&  _on_finished)
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

    PoolWorkersImpl::PoolWorkersImpl(size_t                              _min_workers,
                                     size_t                              _max_workers,
                                     const uint32_t                      _idle_timeout,
                                     const std::optional<size_t>         _max_tasks_count,
                                     xworker::OnThreadStartedFunction&&  _on_started,
                                     xworker::OnThreadFinishedFunction&& _on_finished,
                                     bool                                _fast_on_idle)
        : min_workers_(_min_workers),
          max_workers_(std::max<size_t>(1, _max_workers)),
          idle_timeout_(_idle_timeout),
          max_tasks_count_(_max_tasks_count),
          on_started_pf_(std::move(_on_started)),
          on_finished_pf_(std::move(_on_finished)),
          fast_on_idle_(_fast_on_idle),
          tasks_queue_(CreateTaskQueue()),
          clock_expired_(xclock::Create(xclock::HighResSyncGen(false), 0))
    {
        assert(_max_workers > 0);
        // Add minimum workers w/o timeout -> always running - no expiration
        for (size_t z = 0; z < _min_workers; ++z)
            AddWorker_(std::nullopt);
    }

    IWorker::TaskUid PoolWorkersImpl::TaskPut(TaskFunction&&                            _task_pf,
                                              const std::optional<TaskUid>              _task_uid,
                                              const std::optional<State>                _required_state_mask,
                                              std::optional<std::promise<FinishType>>&& _task_finish_promise)
    {
        assert(_task_uid.value_or(~xbase::kInvalidUid) != xbase::kInvalidUid);

        std::shared_lock lck_r(rw_);

        if (executing_now_p_->load() < workers_.size()) {
            // Try execute task on current workers
            auto task_uid = ExecuteTask_(std::move(_task_pf), _task_uid, std::move(_task_finish_promise));
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

            auto added = worker_p->TaskPut(TaskWithCounter_(_task_pf), _task_uid, {}, std::move(_task_finish_promise));
            assert(added != xbase::kInvalidUid);
            // Set preferred only if has specified task uid (?)
            assert(task_uid == xbase::kInvalidUid || task_uid == added);
            if (task_uid != xbase::kInvalidUid)
                preferred_workers_.SetPreferredWorker(task_uid, idx, worker_p->ThreadId());

            return added;
        }

        if (tasks_queue_->Size() >= max_tasks_count_.value_or(xbase::npos))
            return xbase::kInvalidUid;

        ITasksQueue::Task task = {_task_uid.value_or(xbase::kInvalidUid),
                                  std::move(_task_pf),
                                  std::move(_task_finish_promise)};

        auto task_uid = tasks_queue_->EmplaceBack(true, std::move(task));
        assert(task_uid != xbase::kInvalidUid);

        if (fast_on_idle_ && executing_now_p_->load() < workers_.size()) {
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

        return {executing_now_p_->load(), tasks_queue_->Size(), ThreadState_()};
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

        preferred_workers_.Clear();

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
        preferred_workers_.Clear();
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

    inline IWorker::TaskFunction PoolWorkersImpl::TaskWithCounter_(const IWorker::TaskFunction& _task_pf) const
    {
        assert(_task_pf);
        if (!_task_pf)
            return _task_pf;

        return [holder = counter::CreateHolder(executing_now_p_), _task_pf] { return _task_pf(); };
    }

    inline IWorker::State PoolWorkersImpl::ThreadState_() const
    {
        if (!tasks_queue_->Empty() || executing_now_p_->load() >= max_workers_)
            return State::kBusy;

        return workers_.empty() ? State::kStopped : State::kIdle;
    }

    size_t PoolWorkersImpl::FreeExpiredWorkers_() const
    {
        size_t removed = 0;

        auto it = workers_.rbegin();
        while (it != workers_.rend() && workers_.size() > executing_now_p_->load() + min_workers_) {
            if ((*it)->WorkerState() == State::kStopped) {
                it = decltype(it)(workers_.erase(std::next(it).base()));
                ++removed;
                continue;
            }

            ++it;
        }

        return removed;
    }

    void PoolWorkersImpl::OnIdle_(IWorker* _idle_worker_p)
    {
        // Check first w/o lock for better performance
        // Theretically right after next check, task could be added into queue and await free worker
        // but this is handled in TaskPut() latest block (*)
        if (fast_on_idle_ && tasks_queue_->Empty()) {
            if (clock_expired_->ResetLap(kCheckExpiredPeriod64).first) {
                const std::unique_lock lck(rw_);

                FreeExpiredWorkers_();
                preferred_workers_.FreeExpiredPreference(kNonActivePreferenceTimeout, workers_);
            }

            return;
        }

        const std::unique_lock lck(rw_);

        if (clock_expired_->ResetLap(kCheckExpiredPeriod64).first) {
            FreeExpiredWorkers_();
            preferred_workers_.FreeExpiredPreference(kNonActivePreferenceTimeout, workers_);
        }

        if (tasks_queue_->Empty())
            return;

        auto task = tasks_queue_->TakeFront();
        assert(task.has_value());
        if (task.has_value()) {
            auto added = _idle_worker_p->TaskPut(TaskWithCounter_(task->task_pf),
                                                 task->task_uid,
                                                 IWorker::State::kIdle,
                                                 std::move(task->finish_promise));
            if (added == xbase::kInvalidUid) {
                // Worker could be already busy, return task to deque
                tasks_queue_->EmplaceFront(true, std::move(task.value()));
            }
        }
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
                                                   const std::optional<TaskUid>              _task_uid,
                                                   std::optional<std::promise<FinishType>>&& _task_finish_promise)
    {
        auto task_uid = _task_uid.value_or(xbase::kInvalidUid);

        // Try preferred (recent) worker
        if (task_uid != xbase::kInvalidUid) {
            auto added = preferred_workers_.CallPreferredWorker(task_uid, workers_, [&](IWorker* const worker_p) {
                return worker_p->TaskPut(TaskWithCounter_(_task_pf),
                                         _task_uid,
                                         IWorker::State::kIdle,
                                         std::move(_task_finish_promise));
            });
            if (added != xbase::kInvalidUid)
                return added;
        }

        std::vector<std::pair<size_t, IWorker*>> stopped_workers;

        for (size_t idx = 0; idx < workers_.size(); ++idx) {
            auto* worker_p = workers_[idx].get();

            auto added = worker_p->TaskPut(TaskWithCounter_(_task_pf),
                                           _task_uid,
                                           IWorker::State::kIdle,
                                           std::move(_task_finish_promise));
            if (added != xbase::kInvalidUid) {
                // Set preferred only if has specified task uid (?)
                assert(task_uid == xbase::kInvalidUid || task_uid == added);
                if (task_uid != xbase::kInvalidUid)
                    preferred_workers_.SetPreferredWorker(task_uid, idx, worker_p->ThreadId());

                return added;
            }

            // If worker is closed -> try in next cicle
            if (worker_p->WorkerState() == IWorker::State::kStopped) {
                stopped_workers.emplace_back(idx, worker_p);
                continue;
            }
        }

        // Try from stopped workers
        for (const auto [idx, worker_p] : stopped_workers) {

            auto added = worker_p->TaskPut(TaskWithCounter_(_task_pf),
                                           _task_uid,
                                           IWorker::State::kIdle | IWorker ::State::kStopped,
                                           std::move(_task_finish_promise));
            if (added != xbase::kInvalidUid) {
                // Set preferred only if has specified task uid (?)
                assert(task_uid == xbase::kInvalidUid || task_uid == added);
                if (task_uid != xbase::kInvalidUid)
                    preferred_workers_.SetPreferredWorker(task_uid, idx, worker_p->ThreadId());

                return added;
            }
        }

        // No free workers
        return xbase::kInvalidUid;
    }

} // namespace xbase::impl
} // namespace xsdk
