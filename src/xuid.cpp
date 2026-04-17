#include "xbase/xuid.h"

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <shared_mutex>

// Custom specialization of std::hash can be injected in namespace std.
template <>
struct std::hash<std::pair<uint64_t, uint64_t>> {
    std::size_t operator()(const std::pair<uint64_t, uint64_t>& pair) const noexcept
    {
        std::size_t h1 = std::hash<uint64_t> {}(pair.first);
        std::size_t h2 = std::hash<uint64_t> {}(pair.second);
        return h1 ^ (h2 << 1); // or use boost::hash_combine
    }
};

namespace xsdk {

namespace {

    // todo: move to common heleprs ?
    // Note: Lambda called under write mutex lock.
    template <typename TMap>
    std::pair<typename TMap::mapped_type, bool> MapSharedGet(std::shared_mutex&                            _rw_mtx,
                                                             TMap&                                         _map,
                                                             const typename TMap::key_type&                _key,
                                                             std::function<typename TMap::mapped_type()>&& _create_pf)
    {
        {
            const std::shared_lock lck_r(_rw_mtx);

            auto it = _map.find(_key);
            if (it != _map.end())
                return {it->second, false};
        }

        if (!_create_pf)
            return {typename TMap::mapped_type(), false};

        const std::unique_lock lck_w(_rw_mtx);

        auto [it, success] = _map.try_emplace(_key, _create_pf());
        return {it->second, success};
    }

    template <typename TKey>
    class UidChains {
        std::shared_mutex rw_;
        // todo: check map/unordered_map performance
        std::unordered_map<TKey, uint64_t> nexts_;

    public:
        // 2Think: Prev method ?
        uint64_t NextFor(const TKey& _base)
        {
            return MapSharedGet(rw_, nexts_, _base, []() { return xbase::NextUid(); }).first;
        }
    };
} // namespace

uint64_t xbase::NextUid()
{
    static std::atomic<uint64_t> counter = {kFirstUid};
    return counter.fetch_add(1);
}

uint64_t xbase::MakeUid(const uint64_t _group_uid)
{
    if (_group_uid == xbase::kInvalidUid)
        return xbase::NextUid();

    static UidChains<uint64_t> nexts;
    return nexts.NextFor(_group_uid);
}

uint64_t xbase::MakeUid(const uint64_t _uid_first, const uint64_t _uid_second)
{
    if (_uid_first == xbase::kInvalidUid && _uid_second == xbase::kInvalidUid)
        return xbase::NextUid();

    static UidChains<std::pair<uint64_t, uint64_t>> next_pairs;
    return next_pairs.NextFor(std::make_pair(_uid_first, _uid_second));
}

uint64_t xbase::MakeUid(const std::string& _string_base)
{
    static UidChains<std::string> nexts;
    return nexts.NextFor(_string_base);
}

} // namespace xsdk
