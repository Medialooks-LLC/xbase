#pragma once

#include <functional>
#include <future>
#include <memory>
#include <optional>

#include "xpointers.hpp"
#include "xuid.h"

namespace xsdk {
/**
 * @file xworker.h
 * @brief Header file for the IWorker interface and related functions.
 *
 * The main purpose of the `IWorker` class is to represent a simple worker or the pool of worker, allowing developers
 * to add tasks for the worker/to the pool and manage the state of the threads. The interface provides methods to add
 * tasks, manage thread states, cancel tasks, join the threads of worker/pool, and get various thread-related
 * information.
 *
 * Since both worker and pool have the same interface, developers can easily switch between the two.
 */
namespace xbase {
    /**
     * @brief Abstract class for worker thread.
     */
    class IWorker: public xbase::PtrBase<IWorker> {
    public:
        /**
         * @brief Unique identifier of task.
         * @details Used xbase::NextUid() for generate tasks uids.
         */
        using TaskUid = uint64_t;
        /**
         * @enum FinishType
         * @brief Type of task finish.
         */
        enum class FinishType {
            kNormal,    ///< Normal finish.
            kCanceled,  ///< Task was canceled.
            kReplaced,  ///< Task was replaced.
            kPushedOut, ///< Task was pushed out.
            kAbandoned  ///< Task was abandoned.
        };
        /**
         * @enum RepeatType
         * @brief Repeat type of task.
         */
        enum class RepeatType {
            kDoNotRepeat,      ///<  Task should not be repeated.
            kRepeatUntilNext,  ///< Task should be repeated until next will be put to queue.
            kRepeatUntilCancel ///< Task should be repeated until canceled.
        };
        /**
         * @enum State
         * @brief Worker state.
         */
        enum class State {
            kStopped = 1, ///<  Worker is stopped. Working thread was destroyed.
            kIdle    = 2, ///< Worker is idle. Working thread is ready and waiting new tasks.
            kBusy    = 4  ///< Worker is busy.
        };
        /// @brief Function type of task.
        using TaskFunction = std::function<RepeatType()>;

    public:
        virtual ~IWorker() = default;

        /**
         * @brief Function to submit a new task to the worker or the workers pool.
         *
         * @details This function submits a new task to the worker or the workers pool. The function takes an optional
         * task UID, an optional state mask, and an optional promise for the task finish event. If a task UID is
         * specified, the pool will attempt to execute this task in the same thread if it's possible, otherwise, it will
         * be assigned to any available worker. The function returns the task UID on success or `xbase::kInvalidUid` if
         * an error occurred, such as the task with the same ID already exists, or the worker has joined, or the
         * maximum number of tasks in the pool has been reached.
         *
         * @param _task_pf A move-rvalue reference to the task function.
         * @param _task_uid An optional move-rvalue reference to the task UID. If not specified, the pool will
         * generate a new UID for the task.
         * @param _required_state_mask An optional move-rvalue reference to the state mask. If not specified,
         * the task can be executed in any worker state.
         * @param _task_finish_promise An optional move-rvalue reference to the promise for the task finish
         * event.
         *
         * @return The task UID on success or `xbase::kInvalidUid` if an error occurred.
         */
        virtual TaskUid TaskPut(TaskFunction&&                            _task_pf,
                                const std::optional<TaskUid>              _task_uid            = {},
                                const std::optional<State>                _required_state_mask = {},
                                std::optional<std::promise<FinishType>>&& _task_finish_promise = {}) = 0;
        /**
         * @brief Get tasks count.
         * @return A pair of executing and scheduled tasks count.
         */
        virtual std::pair<size_t, size_t> TasksCount() const = 0;

        /**
         * @brief Get maximum tasks count.
         * @return Maximum tasks count.
         * @note When size not set it means that it doesn't has limit for tasks count, but in real life it value is
         * xbase::npos.
         */
        virtual std::optional<size_t> MaxTasks() const = 0;

        /**
         * @brief Get current thread id of worker.
         * @return The ID of the worker thread, or an empty ID if it is not running.
         * @note Not appliable for pool of workers.
         */
        virtual std::thread::id ThreadId() const = 0;
        /**
         * @brief Get current worker state.
         * @return Worker state. @see IWorker::State
         */
        virtual State WorkerState() const = 0;

        /**
         * @brief Enumeration of possible cancel results.
         * @see IWorker::TaskCancel
         */
        enum class CancelRes {
            kInvalidUid,         ///< Invalid task unique identifier.
            kCanceled,           ///< Task was canceled.
            kNotFound,           ///< Task was not found.
            kExecutingNow,       ///< Task is currently being executed.
            kCancelFromExecution ///< The task canceled from execition thread.
        };
        /**
         * @brief Cancel task.
         * @param _task_uid Task unique identifier.
         * @return Cancel result and future of task finish.
         * @see IWorker::CancelRes
         */
        virtual std::pair<CancelRes, std::future<FinishType>> TaskCancel(const TaskUid _task_uid) = 0;

        /**
         * @brief Cancel all task.
         * @return number of canceled tasks, active task finish future
         */
        virtual size_t TaskCancelAll() = 0;

        /**
         * @brief Joins all the worker threads and clears the task queue if requested.
         * @param _cancel_tasks If true, all the tasks in the queue that have been
         * cancelled will be removed before joining the worker threads.
         * @return Returns true.
         */
        virtual bool Join(const bool _cancel_tasks) = 0; // 2Think: maybe change return type to void?
    };

    XENUM_OPS32(IWorker::State)
} // namespace xbase

namespace xworker {
    using namespace xbase;

    static constexpr size_t   kWorkersPoolMinSize         = 4;
    static constexpr size_t   kWorkersPoolMaxSize         = 128;
    static constexpr uint32_t kWorkersPoolIdleTimeoutMsec = 3000;

    /// @brief default workers pool.
    xbase::IWorker* StaticPool();
    /// @brief return _worker or static workers pool (never null)
    inline xbase::IWorker* DefaultWorker(xbase::IWorker* const _worker_p)
    {
        return _worker_p ? _worker_p : StaticPool();
    }

    /**
     * @brief Type alias for a function accepting an IWorker pointer which descride what worker should do on idle.
     */
    using OnIdleFunction           = std::function<void(IWorker* _this)>;
    using OnThreadStartedFunction  = std::function<void(const IWorker* _this)>;
    using OnThreadFinishedFunction = std::function<void(const IWorker* _this)>;
    /**
     * @brief Creates and sets up a new IWorker instance with optional configuration.
     * @param _on_idle: An optional OnIdleFunction to be called when the worker is idle.
     * @param _idle_timeout_msec: An optional idle timeout (in milliseconds).
     * @param _max_tasks_count: An optional maximum number of tasks the worker may have in task queue.
     * @return An IWorker::UPtr to the created and initialized worker instance.
     */
    IWorker::UPtr CreateWorker(OnIdleFunction&&              _on_idle           = {},
                               const std::optional<uint32_t> _idle_timeout_msec = {},
                               const std::optional<size_t>   _max_tasks_count   = {},
                               OnThreadStartedFunction&&     _on_started        = {},
                               OnThreadFinishedFunction&&    _on_finished       = {});

    /**
     * @brief Creates and sets up a new IWorker instance with optional configuration.
     * @param _on_idle: An optional OnIdleFunction to be called when the worker is idle.
     * @param _idle_timeout_msec: An optional idle timeout (in milliseconds).
     * @param _max_tasks_count: An optional maximum number of tasks the worker may have in task queue.
     * @return An IWorker::UPtr to the created and initialized worker instance.
     */
    std::pair<IWorker::UPtr, IWorker::TaskUid> CreateWorkerWithTask(
        IWorker::TaskFunction&&       _worker_task,
        OnIdleFunction&&              _on_idle           = {},
        const std::optional<uint32_t> _idle_timeout_msec = {},
        const std::optional<size_t>   _max_tasks_count   = {},
        OnThreadStartedFunction&&     _on_started        = {},
        OnThreadFinishedFunction&&    _on_finished       = {});

    static constexpr size_t   kDefPoolMinSize         = 1;
    static constexpr size_t   kDefPoolMaxSize         = 32;
    static constexpr uint32_t kDefPoolIdleTimeoutMsec = 3000;
    /**
     * @brief Creates and sets up a pool of worker instances with optional configuration.
     * @param _min_workers The minimum number of workers in the pool.
     * @param _max_workers The maximum number of workers in the pool.
     * @param _idle_timeout_msec An optional idle timeout (in milliseconds).
     * @param _max_tasks_count An optional maximum number of tasks the pool can handle concurrently.
     * @return An unique pointer to the created and initialized pool of workers.
     */
    IWorker::UPtr CreatePool(const size_t                _min_workers       = kDefPoolMinSize,
                             const size_t                _max_workers       = kDefPoolMaxSize,
                             const uint32_t              _idle_timeout_msec = kDefPoolIdleTimeoutMsec,
                             const std::optional<size_t> _max_tasks_count   = {},
                             OnThreadStartedFunction&&   _on_started        = {},
                             OnThreadFinishedFunction&&  _on_finished       = {});

    // Execute sync in specified IWorker context (for keep execution in one thread context)
    // Note: If _pXThread is nullptr -> execute in calling thread
    /**
     * @brief Execute a function synchronously within a specified IWorker context.
     * @tparam TResult The return type of the task function.
     * @param _worker_p A pointer to the IWorker instance.
     * @param _pf The function to execute.
     * @param _required_state_mask An optional mask of required states for the worker.
     * @param _task_uid An optional task UID for the task.
     * @return An optional value of type TResult, if the function was successful; otherwise, std::nullopt.
     */
    template <typename TResult>
    std::optional<TResult> ExecuteSync(IWorker*                            _worker_p,
                                       std::function<TResult()>&&          _pf,
                                       const std::optional<IWorker::State> _required_state_mask = {},
                                       const std::optional<uint64_t>       _task_uid            = {})
    {
        assert(_pf);
        if (!_worker_p || _worker_p->ThreadId() == std::this_thread::get_id())
            return _pf();

        std::promise<IWorker::FinishType> promise;
        auto                              future   = promise.get_future();
        TResult                           result   = {};
        auto                              task_uid = _worker_p->TaskPut(
            [&result, pf = std::move(_pf)]() {
                result = pf();
                return IWorker::RepeatType::kDoNotRepeat;
            },
            std::move(_task_uid),
            std::move(_required_state_mask),
            std::move(promise));
        if (task_uid == xbase::kInvalidUid || !future.valid() || future.get() != IWorker::FinishType::kNormal)
            return std::nullopt;

        return result;
    }

    /**
     * @brief Execute a void function synchronously within a specified IWorker context.
     * @param _worker_p A pointer to the IWorker instance.
     * @param _pf The function to execute.
     * @param _required_state_mask An optional mask of required states for the worker.
     * @param _task_uid An optional task UID for the task.
     */
    bool ExecuteSyncVoid(IWorker*                            _worker_p,
                         std::function<void()>&&             _pf,
                         const std::optional<IWorker::State> _required_state_mask = {},
                         const std::optional<uint64_t>       _task_uid            = {});

    /**
     * @brief Execute a non-void function asynchronously within a specified IWorker context.
     * @tparam TResult The return type of the function.
     * @param _worker_p A pointer to the IWorker instance.
     * @param _pf The function to execute.
     * @param _task_uid An optional task UID for the task.
     * @param _required_state_mask An optional mask of required states for the worker.
     * @return A future representing the result of the executed function.
     */
    template <typename TResult, std::enable_if_t<!std::is_same_v<TResult, IWorker::RepeatType>, bool> = true>
    std::future<TResult> ExecuteAsync(IWorker*                              _worker_p,
                                      std::function<TResult()>&&            _pf,
                                      const std::optional<IWorker::TaskUid> _task_uid            = {},
                                      const std::optional<IWorker::State>   _required_state_mask = {})
    {
        assert(_worker_p && _pf);
        if (!_worker_p || !_pf)
            return {};

        // Use shared_ptr<> as std::function can only be constructed from functors that are copyable
        auto promise_sp = std::make_shared<std::promise<TResult>>();
        auto future     = promise_sp->get_future();
        auto task_uid   = _worker_p->TaskPut(
            [promise_sp, f = std::move(_pf)]() mutable {
                auto res = f();
                promise_sp->set_value(std::move(res));
                return IWorker::RepeatType::kDoNotRepeat;
            },
            std::move(_task_uid),
            std::move(_required_state_mask));
        if (task_uid == xbase::kInvalidUid)
            return {};

        return future;
    }

    /**
     * @brief Execute a function asynchronously within a specified IWorker context,
     *        returning a result of type IWorker::RepeatType.
     *
     * @param _worker_p A pointer to the IWorker instance.
     * @param _pf The function to execute.
     * @param _task_uid An optional task UID for the task.
     * @param _required_state_mask An optional mask of required states for the worker.
     * @return A future representing the result (repeat type) of the executed function.
     */
    std::future<IWorker::FinishType> ExecuteAsyncVoid(IWorker*                               _worker_p,
                                                      std::function<IWorker::RepeatType()>&& _pf,
                                                      const std::optional<IWorker::TaskUid>  _task_uid            = {},
                                                      const std::optional<IWorker::State>    _required_state_mask = {});
} // namespace xworker

} // namespace xsdk
