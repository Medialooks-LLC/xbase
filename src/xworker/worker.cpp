#include "worker.h"

namespace xsdk {

xbase::IWorker::UPtr xworker::CreateWorker(OnIdleFunction&&               _on_idle,
                                           const std::optional<uint32_t>& _idle_timeout_msec,
                                           const std::optional<size_t>&   _max_tasks_count)
{
    return std::make_unique<xbase::impl::WorkerImpl>(std::move(_on_idle), _idle_timeout_msec, _max_tasks_count);
}

std::pair<xbase::IWorker::UPtr, xbase::IWorker::TaskUid> xworker::CreateWorkerWithTask(
    IWorker::TaskFunction&&        _worker_task,
    OnIdleFunction&&               _on_idle,
    const std::optional<uint32_t>& _idle_timeout_msec,
    const std::optional<size_t>&   _max_tasks_count)
{
    auto worker_p = CreateWorker(std::move(_on_idle), _idle_timeout_msec, _max_tasks_count);
    auto task_uid = worker_p->TaskPut(std::move(_worker_task));
    return {std::move(worker_p), task_uid};
}

namespace xbase::impl {

    WorkerImpl::WorkerImpl(xworker::OnIdleFunction&&      _on_idle,
                           const std::optional<uint32_t>& _idle_timeout_msec,
                           const std::optional<size_t>&   _max_tasks_count)
        : on_idle_pf_(_on_idle),
          idle_timeout_msec_(_idle_timeout_msec),
          max_tasks_count_(_max_tasks_count),
          tasks_queue_(CreateTaskQueue())
    {
    }

    WorkerImpl::~WorkerImpl()
    {
        WorkerImpl::Join(true);
        JoinExpired_();
    }

    IWorker::TaskUid WorkerImpl::TaskPut(TaskFunction&&                            _task_pf,
                                         std::optional<TaskUid>&&                  _task_uid,
                                         std::optional<State>&&                    _required_state_mask,
                                         std::optional<std::promise<FinishType>>&& _task_finish_promise)
    {
        assert(!_task_uid.has_value() || _task_uid.value() != xbase::kInvalidUid);

        const std::unique_lock lck(mtx_);

        if (joined_.load() || TasksTotal_() > max_tasks_count_.value_or(xbase::npos))
            return xbase::kInvalidUid;

        if (_required_state_mask.has_value() && !(_required_state_mask.value() & WorkerState()))
            return xbase::kInvalidUid;

        if (!worker_thread_p_)
            worker_thread_p_ = std::make_unique<std::thread>(&WorkerImpl::ThreadRun_, this);

        ITasksQueue::Task task = {_task_uid.value_or(xbase::kInvalidUid),
                                  std::move(_task_pf),
                                  std::move(_task_finish_promise)};

        auto task_uid = tasks_queue_->EmplaceBack(true, std::move(task));
        assert(task_uid != xbase::kInvalidUid);
        have_tasks_.notify_all();
        return task_uid;
    }
    std::pair<size_t, size_t> WorkerImpl::TasksCount() const
    {
        return {executed_task_id_.load() != xbase::kInvalidUid ? 1 : 0, tasks_queue_->Size()};
    };

    std::thread::id WorkerImpl::ThreadId() const
    {
        const std::unique_lock lck(mtx_);

        return worker_thread_p_ ? worker_thread_p_->get_id() : std::thread::id {};
    }

    std::pair<IWorker::CancelRes, std::future<IWorker::FinishType>> WorkerImpl::TaskCancel(const TaskUid _task_uid)
    {
        if (_task_uid == xbase::kInvalidUid)
            return {CancelRes::kInvalidUid, std::future<IWorker::FinishType> {}};

        const std::unique_lock lck(mtx_);
        if (executed_task_id_.load() == _task_uid) {
            auto cancel_future = tasks_queue_->CancelMarkAdd(_task_uid);
            return {CancelRes::kExecutingNow, std::move(cancel_future)};
        }

        if (tasks_queue_->Erase(_task_uid, FinishType::kCanceled))
            return {CancelRes::kCanceled, std::future<IWorker::FinishType> {}};

        return {CancelRes::kNotFound, std::future<IWorker::FinishType> {}};
    }

    size_t WorkerImpl::TaskCancelAll()
    {
        std::unique_lock lck(mtx_);

        size_t canceled = tasks_queue_->EraseAll(FinishType::kCanceled);

        if (executed_task_id_.load() == xbase::kInvalidUid)
            return canceled;

        auto cancel_future = tasks_queue_->CancelMarkAdd(executed_task_id_.load());
        ++canceled;

        lck.unlock();
        
        if (cancel_future.valid())
            cancel_future.wait();

        return canceled;
    }

    bool WorkerImpl::Join(const bool _cancel_tasks)
    {
        std::unique_lock lck(mtx_);

        if (_cancel_tasks) {
            tasks_queue_->EraseAll(FinishType::kCanceled);

            // Check for current task, if have one, cancel it's
            auto executed_task_id = executed_task_id_.load();
            if (executed_task_id != xbase::kInvalidUid)
                tasks_queue_->CancelMarkAdd(executed_task_id);
        }

        joined_.store(true);

        have_tasks_.notify_all();
        auto thread_p = std::exchange(worker_thread_p_, std::unique_ptr<std::thread>());
        lck.unlock();

        if (thread_p && thread_p->joinable()) {
            assert(thread_p->get_id() != std::this_thread::get_id());
            thread_p->join();
            // if (thread_p->get_id() != std::this_thread::get_id())
            //     thread_p->join();
            // else
            //     thread_p->detach();
        }

        return true;
    }

    IWorker::State WorkerImpl::WorkerState() const
    {
        if (executed_task_id_.load() != xbase::kInvalidUid || !tasks_queue_->Empty())
            return State::kBusy;

        return worker_thread_p_ ? State::kIdle : State::kStopped;
    }

    size_t WorkerImpl::TasksTotal_() const
    {
        return tasks_queue_->Size() + (executed_task_id_.load() != xbase::kInvalidUid ? 1 : 0);
    }

    bool WorkerImpl::WaitForTasks_(std::unique_lock<std::mutex>& _lck, std::optional<uint32_t> _max_wait_msec)
    {
        if (_max_wait_msec.has_value()) {
            if (!have_tasks_.wait_for(_lck, std::chrono::milliseconds(_max_wait_msec.value()), [&]() {
                    return !tasks_queue_->Empty() || joined_.load();
                })) {
                return false;
            }
        }
        else {
            have_tasks_.wait(_lck, [&]() { return !tasks_queue_->Empty() || joined_.load(); });
        }

        return !tasks_queue_->Empty();
    }

    bool WorkerImpl::JoinExpired_()
    {
        if (!expired_thread_p_)
            return false;

        assert(expired_thread_p_->get_id() != std::this_thread::get_id() && expired_thread_p_->joinable());
        expired_thread_p_->join();
        expired_thread_p_.reset();

        return true;
    }

    void WorkerImpl::ThreadRun_()
    {
        JoinExpired_();

        while (true) {
            std::unique_lock lck(mtx_);

            auto next_task = tasks_queue_->TakeFront();
            if (!next_task.has_value()) {
                // Mark worker as idle (for ability to add tasks with idle check)
                executed_task_id_.store(xbase::kInvalidUid);

                if (on_idle_pf_ && !joined_.load()) {
                    lck.unlock();
                    on_idle_pf_(this);
                    lck.lock();
                    // Task could be added during callback
                    next_task = tasks_queue_->TakeFront();
                }

                if (!next_task.has_value()) {

                    if (joined_.load() || !WaitForTasks_(lck, idle_timeout_msec_)) {
                        // Stop thread by timeout (if joined - worker_thread_p_ is nullptr)
                        if (worker_thread_p_ && worker_thread_p_->get_id() == std::this_thread::get_id()) {
                            assert(!expired_thread_p_);
                            expired_thread_p_ = std::exchange(worker_thread_p_, std::unique_ptr<std::thread>());
                        }
                        break;
                    }
                    continue;
                }
            }

            executed_task_id_.store(next_task->task_uid);
            lck.unlock();

            while (true) {
                auto repeat = next_task->task_pf();
                if (repeat != RepeatType::kDoNotRepeat && tasks_queue_->Empty() && !joined_.load())
                    continue;

                lck.lock();
                if (repeat == RepeatType::kRepeatUntilCancel) {
                    // If task canceled or replaced -> EmplaceBack() failed and task & cancel promise set inside
                    // EmplaceBack() call
                    tasks_queue_->EmplaceBack(false, std::move(next_task.value()));
                }
                else {
                    // Task finished
                    if (next_task->finish_promise.has_value()) {
                        next_task->finish_promise->set_value(
                            repeat == RepeatType::kRepeatUntilNext ? FinishType::kPushedOut : FinishType::kNormal);
                    }
                    // Remove cancel mark (for set promise)
                    tasks_queue_->CancelMarkRemove(next_task->task_uid, FinishType::kCanceled);
                }
                lck.unlock();
                break;
            }
        }
    }
} // namespace xbase::impl
} // namespace xsdk