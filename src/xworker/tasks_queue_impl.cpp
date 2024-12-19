#include "tasks_queue_impl.h"

namespace xsdk::xbase::impl {

ITasksQueue::UPtr CreateTaskQueue() { return ITasksQueue::UPtr {new TasksQueueImpl()}; }

TasksQueueImpl::~TasksQueueImpl() { TasksQueueImpl::EraseAll(IWorker::FinishType::kAbandoned); }

std::optional<ITasksQueue::Task> TasksQueueImpl::TakeFront()
{
    while (!tasks_.empty()) {
        auto task = std::move(tasks_.front());
        tasks_.pop_front();
        deq_size_.store(tasks_.size());

        // Check for cancel mark - next situation may occurs if cancel mark replace existed task
        if (!task.task_pf) {
            SetPromise_(task, IWorker::FinishType::kCanceled);
            continue;
        }

        return task;
    }

    return std::nullopt;
}

IWorker::TaskUid TasksQueueImpl::EmplaceBack(const bool _replace, ITasksQueue::Task&& _task)
{
    return EmplaceFrontOrBack_(DeqDest::kBack, _replace, std::move(_task));
}

IWorker::TaskUid TasksQueueImpl::EmplaceFront(const bool _replace, ITasksQueue::Task&& _task)
{
    return EmplaceFrontOrBack_(DeqDest::kFront, _replace, std::move(_task));
}

std::future<IWorker::FinishType> TasksQueueImpl::CancelMarkAdd(const IWorker::TaskUid _task_uid)
{
    // If have task with same id -> cancel it's
    Erase(_task_uid, IWorker::FinishType::kCanceled);

    std::promise<IWorker::FinishType> cancel_promise;
    auto                              cancel_future = cancel_promise.get_future();

    tasks_.push_back(ITasksQueue::Task {_task_uid, {}, std::move(cancel_promise)});
    deq_size_.store(tasks_.size());
    return cancel_future;
}

bool TasksQueueImpl::CancelMarkRemove(const IWorker::TaskUid _task_uid, IWorker::FinishType _promise_value)
{
    auto it = FindTask_(_task_uid);
    if (it == tasks_.end() || it->task_pf)
        return false;

    SetPromise_(*it, _promise_value);

    tasks_.erase(it);
    deq_size_.store(tasks_.size());
    return true;
}

bool TasksQueueImpl::Erase(const IWorker::TaskUid _task_uid, IWorker::FinishType _promise_value)
{
    auto it = FindTask_(_task_uid);
    if (it == tasks_.end())
        return false;

    SetPromise_(*it, _promise_value);

    tasks_.erase(it);
    deq_size_.store(tasks_.size());
    return true;
}

size_t TasksQueueImpl::EraseAll(IWorker::FinishType _promise_value)
{
    for (auto& task : tasks_)
        SetPromise_(task, _promise_value);

    deq_size_.store(0);
    return std::exchange(tasks_, std::deque<ITasksQueue::Task>()).size();
}

IWorker::TaskUid TasksQueueImpl::EmplaceFrontOrBack_(const DeqDest       _endpoint,
                                                     const bool          _replace,
                                                     ITasksQueue::Task&& _task)
{
    if (_task.task_uid == xbase::kInvalidUid)
        _task.task_uid = xbase::NextUid();

    auto it = FindTask_(_task.task_uid);
    if (it != tasks_.end()) {
        if (!it->task_pf) {
            // Task canceled
            SetPromise_(_task, IWorker::FinishType::kCanceled);
            SetPromise_(*it, IWorker::FinishType::kCanceled);
            tasks_.erase(it);
            deq_size_.store(tasks_.size());
            return xbase::kInvalidUid;
        }

        if (!_replace) {
            SetPromise_(_task, IWorker::FinishType::kPushedOut);
            return xbase::kInvalidUid;
        }

        auto replaced_task = std::exchange(*it, std::move(_task));
        SetPromise_(replaced_task, IWorker::FinishType::kReplaced);
        return it->task_uid;
    }

    const auto task_uid = _task.task_uid;
    if (_endpoint == DeqDest::kBack)
        tasks_.emplace_back(std::move(_task));
    else
        tasks_.emplace_front(std::move(_task));

    deq_size_.store(tasks_.size());
    return task_uid;
}

std::deque<ITasksQueue::Task>::iterator TasksQueueImpl::FindTask_(const IWorker::TaskUid _task_uid)
{
    // 2Think: about reverse_interator usage
    return std::find_if(tasks_.begin(), tasks_.end(), [&](const ITasksQueue::Task& task) {
        return task.task_uid == _task_uid;
    });
}

/*static*/ bool TasksQueueImpl::SetPromise_(ITasksQueue::Task& _task, IWorker::FinishType _promise_value)
{
    if (!_task.finish_promise.has_value())
        return false;

    _task.finish_promise->set_value(_promise_value);
    _task.finish_promise.reset();
    return true;
}
} // namespace xsdk::xbase::impl