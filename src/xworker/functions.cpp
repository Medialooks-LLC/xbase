#include "xbase.h"

#include "pool.h"
#include "scheduler.h"
#include "worker.h"

#include <chrono>
#include <thread>

namespace xsdk {

void xworker::SleepMsec(const int32_t _msec)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(_msec));
}

bool xworker::ExecuteSyncVoid(IWorker*                        _worker_p,
                              std::function<void()>&&         _pf,
                              const std::optional<IWorker::State> _required_state_mask,
                              const std::optional<uint64_t>       _task_uid)
{
    assert(_pf);
    if (!_pf)
        return false;

    if (!_worker_p || _worker_p->ThreadId() == std::this_thread::get_id()) {
        _pf();
        return true;
    }

    std::promise<IWorker::FinishType> promise;
    auto                              future   = promise.get_future();
    auto                              task_uid = _worker_p->TaskPut(
        [pf = std::move(_pf)]() {
            pf();
            return IWorker::RepeatType::kDoNotRepeat;
        },
        std::move(_task_uid),
        std::move(_required_state_mask),
        std::move(promise));
    if (task_uid == xbase::kInvalidUid || !future.valid() || future.get() != IWorker::FinishType::kNormal)
        return false;

    return true;
}

std::future<xbase::IWorker::FinishType> xworker::ExecuteAsyncVoid(IWorker*                               _worker_p,
                                                                  std::function<IWorker::RepeatType()>&& _pf,
                                                                  const std::optional<IWorker::TaskUid>      _task_uid,
                                                                  const std::optional<IWorker::State> _required_state_mask)
{
    assert(_worker_p && _pf);
    if (!_worker_p || !_pf)
        return {};

    std::promise<IWorker::FinishType> promise;
    auto                              future   = promise.get_future();
    auto                              task_uid = _worker_p->TaskPut(std::move(_pf),
                                       std::move(_task_uid),
                                       std::move(_required_state_mask),
                                       std::move(promise));
    if (task_uid == xbase::kInvalidUid)
        return {};

    return future;
}

} // namespace xsdk
