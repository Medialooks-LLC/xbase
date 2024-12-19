#pragma once

#include <numeric>
#include <limits>

namespace xsdk::xbase {

// TODO: Move to something like numbers.h ?
template <typename TClamp, typename TNumber, std::enable_if_t<std::is_signed_v<TNumber>, bool> = true>
static constexpr TClamp Clamp(const TNumber _number)
{
    return _number < std::numeric_limits<TClamp>::min() ? std::numeric_limits<TClamp>::min() :
           _number > std::numeric_limits<TClamp>::max() ? std::numeric_limits<TClamp>::max() :
                                                          static_cast<TClamp>(_number);
}

template <typename TClamp, typename TNumber, std::enable_if_t<std::is_unsigned_v<TNumber>, bool> = true>
static constexpr TClamp Clamp(const TNumber _number)
{
    return _number < 0                                                        ? 0 :
           _number > static_cast<TNumber>(std::numeric_limits<TClamp>::max()) ? std::numeric_limits<TClamp>::max():
                                                                                static_cast<TClamp>(_number);
}

} // namespace xsdk::xbase