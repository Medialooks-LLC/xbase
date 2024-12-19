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

class TasksQueueImpl final: public ITasksQueue {

    std::deque<ITasksQueue::Task> tasks_;
    std::atomic<size_t>           deq_size_ = {0};

public:
    TasksQueueImpl() = default;
    ~TasksQueueImpl() override;

    virtual bool                             Empty() const override { return deq_size_.load() == 0; }
    virtual size_t                           Size() const override { return deq_size_.load(); }
    virtual std::optional<ITasksQueue::Task> TakeFront() override;
    virtual IWorker::TaskUid                 EmplaceBack(const bool _replace, ITasksQueue::Task&& _task) override;
    virtual IWorker::TaskUid                 EmplaceFront(const bool _replace, ITasksQueue::Task&& _task) override;
    virtual std::future<IWorker::FinishType> CancelMarkAdd(const IWorker::TaskUid _task_uid) override;
    virtual bool   CancelMarkRemove(const IWorker::TaskUid _task_uid, IWorker::FinishType _promise_value) override;
    virtual bool   Erase(const IWorker::TaskUid _task_uid, IWorker::FinishType _promise_value) override;
    virtual size_t EraseAll(const IWorker::FinishType _promise_value) override;

private:
    enum class DeqDest { kFront, kBack };
    IWorker::TaskUid EmplaceFrontOrBack_(const DeqDest _endpoint, const bool _replace, ITasksQueue::Task&& _task);
    std::deque<ITasksQueue::Task>::iterator FindTask_(const IWorker::TaskUid _task_uid);
    static bool                             SetPromise_(ITasksQueue::Task& _task, IWorker::FinishType _promise_value);
};
} // namespace xsdk::xbase::impl