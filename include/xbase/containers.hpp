#pragma once

#include <algorithm>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <optional>

namespace xsdk::xbase {
/**
 * @brief Find element index in container like std::vector or std::deque, optimized for basic types
 */
template <typename TContainer, typename TElement>
std::optional<size_t> ElementIndex(const TContainer& _container, const TElement& _find)
{
    auto it = std::find(_container.begin(), _container.end(), _find);
    if (it == _container.end())
        return std::nullopt;

    return static_cast<size_t>(std::distance(_container.begin(), it));
}

} // namespace xsdk::xbase

namespace xsdk::xbase::containers {

template <typename TMap>
std::pair<typename TMap::mapped_type, bool> MapSharedGet(
    std::shared_mutex&                            _rw_mtx,
    TMap&                                         _map,
    const typename TMap::key_type&                _key,
    std::function<typename TMap::mapped_type()>&& _create_pf)
{
    {
        const std::shared_lock lock_read(_rw_mtx);
        auto                   it = _map.find(_key);
        if (it != _map.end())
            return {it->second, false};
    }

    if (!_create_pf)
        return {typename TMap::mapped_type(), false};

    const std::unique_lock lock_write(_rw_mtx);
    auto                   it = _map.find(_key);
    if (it != _map.end())
        return {it->second, false};

    auto [new_it, inserted] = _map.try_emplace(_key, _create_pf());
    return {new_it->second, inserted};
}

} // namespace xsdk::xbase::containers
