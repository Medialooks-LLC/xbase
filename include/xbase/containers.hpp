#pragma once

#include <algorithm>
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
