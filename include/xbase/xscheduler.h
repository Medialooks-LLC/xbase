#pragma once

#include <cassert>
#include <functional>
#include <future>
#include <memory>
#include <optional>

#include "xclock.h"
#include "xpointers.hpp"
#include "xworker.h"

namespace xsdk {
namespace xbase {
    /**
     * @brief Interface for scheduler implementation
     * @details This interface defines the functions for scheduling tasks, managing task statuses,
     *          and cancelling scheduled tasks.
     *
     * @par TaskInfo structure
     * The IScheduler::TaskInfo structure stores information related to a task, including the task UID,
     * the scheduled time, scheduling counter, worker busy counter, and a pointer to the scheduler clock.
     *
     * @par TaskRes enumeration
     * The IScheduler::TaskRes enumeration represents the results of scheduling, rescheduling, and cancelling tasks.
     *
     * @par Status enumeration
     * The IScheduler::Status enumeration defines the statuses that a task can have, including not found, scheduled,
     * and executing now.
     */
    class IScheduler: public xbase::PtrBase<IScheduler> {
    public:
        /// @brief Contains information about a task.
        struct TaskInfo {
            /// @brief Unique identifier for a task.
            IWorker::TaskUid task_uid = xbase::kInvalidUid;
            /// @brief The scheduled time for the task
            xbase::Time64 scheduled_time = time64::kNoVal;
            /// @brief Counter for tracking the number of times a task is scheduled, for immediate repeated task do not
            /// increased (private or default workers required for immediate repeat)
            uint64_t scheduling_counter = 0;
            /// @brief Counter for tracking the number of times a worker is busy when task was re-added.
            uint64_t worker_busy_counter = 0;
            /// @brief Pointer to a clock used for time keeping.
            const xbase::IClock* clock_p = nullptr;
        };

        /**
         * @brief A callable object representing a task function.
         * @param _task_info A type that is convertible to `const IScheduler::TaskInfo*`.
         * @return An optional `xbase::Time64` representing the time at which the task should be executed next time.
         */
        using TaskFunction = std::function<std::optional<xbase::Time64>(const IScheduler::TaskInfo* _task_info)>;

        /// @brief An enumeration representing the result of task scheduling.
        enum class TaskRes {
            kInvalidUid, ///< The task identifier is invalid or TaskFunction was not set.
            kOk,         ///< The task was successfully scheduled.
            kNotFound,   ///< The task is not found in the scheduler queue nor in the exexuting tasks. @note Applicable
                         ///< for rescheduling tasks
            kExecutingNow, ///< The task is currently executing. @note Applicable for rescheduling tasks
            kForcedNow,    ///< The task was forced to be executed immediately. @note Applicable for rescheduling tasks
            kWorkerBusy    ///< The worker is currently busy, and the task could not be scheduled.
        };

        /// @brief Task status enumeration
        enum class Status {
            kInvalidUid,  ///< The task identifier is invalid.
            kNotFound,    ///< The task is not found in the scheduler queue nor in the exexuting tasks.
            kScheduled,   ///< The task is scheduled to be executed.
            kExecutingNow ///< The task is currently executing.
        };

    public:
        virtual ~IScheduler() = default;

        /**
         * @brief Get scheduler clock
         * @return A pointer to the scheduler's clock
         * @note Returned value is never null
         */
        virtual const xbase::IClock* Clock() const = 0;

        /**
         * @brief Schedules a task with the given parameters.
         * @param _scheduled_time The scheduled time for the task.
         * @param _task The function to be executed as the task.
         * @param _task_uid The UID of the task, default is an invalid UID.
         * @param _task_worker The worker associated with the task, null if there is no worker.
         * @return The UID of the scheduled task.
         * @note If task with specified uid already exists, operation failed (return xbase::kInvalidUid) (2think)
         */
        virtual IWorker::TaskUid ScheduleTask(const xbase::Time64                    _scheduled_time,
                                              TaskFunction&&                         _task,
                                              const std::optional<IWorker::TaskUid>& _task_uid    = {},
                                              const IWorker::SPtr&                   _task_worker = nullptr) = 0;

        /**
         * @brief Get the status and information for a specified task
         * @param _task_id The UID of the task
         * @return A pair containing the task status and the TaskInfo structure
         * @see IScheduler::Status @see IScheduler::TaskInfo
         * @details This function checks if the task UID is valid. If not, it returns the status `kInvalidUid`
         *          and an empty task information `{}`.
         *
         *          If the task UID is valid, it checks if the task is currently executing. If so, it returns the status
         *          `kExecutingNow` and the task information associated with the executing task.
         *
         *          If the task is not currently executing, it searches for the task in the tasks queue.
         *          If the task is not found, it returns the status `kNotFound` and an empty task information `{}`.
         *
         *          If the task is found in the tasks queue, it returns the status `kScheduled` and the task
         *          information associated with the scheduled task.
         */
        virtual std::pair<Status, TaskInfo> TaskStatus(const IWorker::TaskUid _task_id) const = 0;

        /**
         * @brief Reschedule a task
         * @param _task_id The UID of the task
         * @param _scheduled_time The new time for the task to be executed
         * @return The result of the rescheduling operation
         * @note If there are available workers and the new scheduled time is less than the current time,
         *       the task is immediately executed
         */
        virtual TaskRes RescheduleTask(const IWorker::TaskUid _task_id, const xbase::Time64 _scheduled_time) = 0;

        // 2Think: Rename to RemoveTask() use return std::optional<TaskInfo> + future ?
        /**
         * @brief Cancel a task and return wait future for its completion (if applicable)
         * @param _task_id The UID of the task to be cancelled
         * @return A pair containing the result of the cancellation operation and a future to wait for the task
         * completion
         * @warning Wait future is not applicable for tasks without workers
         */
        virtual std::pair<TaskRes, std::future<IWorker::FinishType>> CancelTask(const IWorker::TaskUid _task_id) = 0;
    };
} // namespace xbase

namespace xscheduler {
    using namespace xbase;

    /// @brief default scheduler (use default workers pool)
    xbase::IScheduler* StaticScheduler();
    /// @brief return _scheduler or static scheduler (never null)
    inline xbase::IScheduler* DefaultScheduler(xbase::IScheduler* const _scheduler_p)
    {
        return _scheduler_p ? _scheduler_p : StaticScheduler();
    }

    /**
     * @brief Creates a new IScheduler instance with the given clock and default worker.
     * @param _clock_p The clock to be used by the scheduler.
     * @param _use_static_workers_pool - Use static workers pool: xworker::StaticPool() for execute scheduled tasks
     * @param _default_worker Pointer to the default worker to be used by tasks (if _use_static_workers_pool is true,
     * ignored, shiould be nullptr)
     * @return An IScheduler UPtr that owns the newly created scheduler instance.
     */
    IScheduler::UPtr CreateScheduler(const xbase::IClock* _clock_p,
                                     const bool           _use_static_workers_pool = true,
                                     const IWorker::SPtr& _default_worker          = {});
 
    /**
     * @brief Schedules a new task with a given delay and function.
     * @tparam TResult The result type of the scheduled task.
     * @param _scheduler_p Pointer to the scheduler instance.
     * @param _msec_delay The delay in milliseconds before the task is executed.
     * @param _pf The function to be executed when the task is scheduled.
     * @param _task_worker Optional pointer to a worker instance to be used for the task.
     * @return A std::pair containing the task UID and a std::future representing the task result.
     */
    template <typename TResult>
    std::pair<IWorker::TaskUid, std::future<TResult>> ScheduleTask(IScheduler*                _scheduler_p,
                                                                   const double               _msec_delay,
                                                                   std::function<TResult()>&& _pf,
                                                                   const IWorker::SPtr&       _task_worker = nullptr)
    {
        assert(_scheduler_p && _pf);
        if (!_scheduler_p || !_pf)
            return {xbase::kInvalidUid, std::future<TResult> {}};

        // Use shared_ptr<> as std::function can only be constructed from functors that are copyable
        auto promise_sp = std::make_shared<std::promise<TResult>>();
        auto future     = promise_sp->get_future();
        auto task_uid   = _scheduler_p->ScheduleTask(
            _scheduler_p->Clock()->Time() + time64::FromMsec(_msec_delay),
            [promise_sp, f = std::move(_pf)](const auto*) mutable {
                auto res = f();
                promise_sp->set_value(std::move(res));
                return std::nullopt; // No repeats;
            },
            {},
            _task_worker);

        return {task_uid, std::move(future)};
    }
} // namespace xscheduler

} // namespace xsdk