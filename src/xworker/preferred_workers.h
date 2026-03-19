#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <shared_mutex>

#include "xbase/xclock.h"
#include "xbase/xtime.h"
#include "xbase/xworker.h"

namespace xsdk::xbase::impl {

class PreferredWorkers {

    class TaskPreference {
        std::atomic<size_t>          worker_idx_cache_   = {xbase::npos};
        std::atomic<xbase::Time64>   activity_timestamp_ = {time64::kNoVal};
        std::atomic<std::thread::id> worker_thread_id_;

    public:
        // For debug/perfomance check
        std::atomic<size_t> succeeded = {0};
        std::atomic<size_t> failed    = {0};

        using UPtr = std::unique_ptr<TaskPreference>;

    public:
        TaskPreference() : activity_timestamp_(xclock::UtcTime()) {}

        //~TaskPreference() { std::cout << (double)succeeded / (succeeded + failed) << std::endl; }

        void     SetWorker(const size_t _idx, const std::thread::id _worker_thread_id);
        IWorker* GetWorker(const std::vector<IWorker::UPtr>& _workers);

        void UpdateActivity() { activity_timestamp_.store(xclock::UtcTime()); }

        xbase::Time64 InactiveTime() const { return xclock::UtcTime() - activity_timestamp_.load(); }
    };

    mutable std::shared_mutex rw_;

    std::map<IWorker::TaskUid, TaskPreference::UPtr> tasks_preference_;

public:
    // Note: expect what _workers at least under read protection
    // TFunc is std::function<xbase::Uid(IWorker* const preferred_worker_p)>, the result of this method if callback call
    // result of xbase::kInvalidUid if no preferred workers
    // WARNING !!! Called under read lock of mutex, any calls for PreferredWorkers from callback is strictly prohibited
    template <typename TFunc>
    xbase::Uid CallPreferredWorker(const xbase::Uid                  _task_uid,
                                   const std::vector<IWorker::UPtr>& _workers,
                                   TFunc&&                           _on_preferred_worker_r_locked_cb)
    {
        const std::shared_lock lck(rw_);

        auto it = tasks_preference_.find(_task_uid);
        if (it == tasks_preference_.end())
            return xbase::kInvalidUid;

        auto* worker_p = it->second->GetWorker(_workers);
        if (worker_p) {
            auto call_res = _on_preferred_worker_r_locked_cb(worker_p);
            if (call_res != xbase::kInvalidUid) {
                it->second->succeeded.fetch_add(1);
                return call_res;
            }
        }

        it->second->failed.fetch_add(1);
        return xbase::kInvalidUid;
    }

    // Note: expect what _workers at least under read protection
    void SetPreferredWorker(const xbase::Uid _task_uid, const size_t _idx, const std::thread::id _worker_thread_id);

    // Note: expect what _workers at least under read protection
    size_t FreeExpiredPreference(const xbase::Time64 _non_active_timeout, const std::vector<IWorker::UPtr>& _workers);

    void Clear();
};

} // namespace xsdk::xbase::impl
