#include "preferred_workers.h"
#include "xbase/xuid.h"

namespace xsdk::xbase::impl {

void PreferredWorkers::TaskPreference::SetWorker(const size_t _idx, const std::thread::id _worker_thread_id)
{
    // Set thread-id first (as main parameter)
    worker_thread_id_.store(_worker_thread_id);
    // Store index for fast GetWorker() call (even if index is wrong/not set yet/context switches, GetWorker() return
    // correct result searching by thread id)
    worker_idx_cache_.store(_idx);
    // Update activity timestamp
    activity_timestamp_.store(xclock::UtcTime());
}

// Note: expect what _workers at least under read protection
IWorker* PreferredWorkers::TaskPreference::GetWorker(const std::vector<IWorker::UPtr>& _workers)
{
    // For update activity timestamp
    const auto time_utc = xclock::UtcTime();

    // Check for cached worker index
    const auto worker_idx       = worker_idx_cache_.load();
    const auto worker_thread_id = worker_thread_id_.load();
    if (worker_idx < _workers.size() && _workers[worker_idx]->ThreadId() == worker_thread_id) {
        activity_timestamp_.store(time_utc);
        return _workers[worker_idx].get();
    }

    // Try find worker with required thread_id
    for (size_t idx = 0; idx < _workers.size(); ++idx) {
        if (_workers[idx]->ThreadId() == worker_thread_id) {
            // Store founded index for optimize next calls
            worker_idx_cache_.store(idx);
            activity_timestamp_.store(time_utc);
            return _workers[idx].get();
        }
    }

    // Preferred worker not found, reset activity time for fast remove at next FreeExpiredPreference_
    activity_timestamp_.store(time_utc - time64::kYear);
    return nullptr;
}

void PreferredWorkers::SetPreferredWorker(const xbase::Uid      _task_uid,
                                          const size_t          _idx,
                                          const std::thread::id _worker_thread_id)
{
    {
        const std::shared_lock lck_r(rw_);

        auto it = tasks_preference_.find(_task_uid);
        if (it != tasks_preference_.end()) {
            it->second->SetWorker(_idx, _worker_thread_id);
            return;
        }
    }

    const std::unique_lock lck_w(rw_);

    auto it = tasks_preference_.emplace(_task_uid, std::make_unique<TaskPreference>()).first;
    assert(it != tasks_preference_.end());
    it->second->SetWorker(_idx, _worker_thread_id);
}

size_t PreferredWorkers::FreeExpiredPreference(const xbase::Time64               _non_active_timeout,
                                               const std::vector<IWorker::UPtr>& _workers)
{
    const std::unique_lock lck(rw_);

    size_t expired = 0;
    auto   it      = tasks_preference_.begin();
    while (it != tasks_preference_.end()) {
        if (it->second->InactiveTime() > _non_active_timeout || !it->second->GetWorker(_workers)) {
            it = tasks_preference_.erase(it);
            ++expired;
            continue;
        }

        ++it;
    }

    return expired;
}

void PreferredWorkers::Clear()
{
    const std::unique_lock lck(rw_);

    tasks_preference_.clear();
}

} // namespace xsdk::xbase::impl
