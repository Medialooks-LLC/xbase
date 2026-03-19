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

#include "xbase.h"

namespace xsdk::xbase::impl {

// Internal interface for tasks queue
class ITasksQueue: public PtrBase<ITasksQueue> {
public:
    struct Task {
        IWorker::TaskUid                                 task_uid = xbase::kInvalidUid;
        IWorker::TaskFunction                            task_pf;
        std::optional<std::promise<IWorker::FinishType>> finish_promise;

        // Only move allowed
        Task()                  = default;
        Task(Task&&)            = default;
        Task& operator=(Task&&) = default;

        // Copy is prohibited
        Task(const Task&)            = delete;
        Task& operator=(const Task&) = delete;
    };

    virtual ~ITasksQueue() = default;

    // Thread safe call
    virtual bool Empty() const = 0;
    // Thread safe call
    virtual size_t              Size() const = 0;
    virtual std::optional<Task> TakeFront()  = 0;
    // If task not emplaced - it's not moved (like std::map::try_emplace)
    virtual IWorker::TaskUid EmplaceBack(const bool _replace, Task&& _task) = 0;
    // If task not emplaced - it's not moved (like std::map::try_emplace)
    virtual IWorker::TaskUid EmplaceFront(const bool _replace, Task&& _task) = 0;
    // For prevent to add task with specified id, then task with specified id is added -> future is set
    virtual std::future<IWorker::FinishType> CancelMarkAdd(const IWorker::TaskUid _task_uid) = 0;
    // Remove cancel mark and set specified value to cancel future
    virtual bool CancelMarkRemove(const IWorker::TaskUid _task_uid, IWorker::FinishType _promise_value) = 0;
    // Erase specified tasks and set finish promise
    virtual bool Erase(const IWorker::TaskUid _task_uid, IWorker::FinishType _promise_value) = 0;
    // Erase all tasks and set finish promise
    virtual size_t EraseAll(const IWorker::FinishType _promise_value) = 0;
};

impl::ITasksQueue::UPtr CreateTaskQueue();

} // namespace xsdk::xbase::impl
