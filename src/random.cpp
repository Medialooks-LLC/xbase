#include "xbase/random.h"

#include <algorithm>
#include <random>

namespace xsdk::xbase::random {
namespace {
    std::mt19937_64& Engine()
    {
        thread_local std::mt19937_64 engine {std::random_device {}()};
        return engine;
    }
} // namespace

double Value()
{
    return std::uniform_real_distribution<double>(0.0, 1.0)(Engine());
}

int32_t Int(const double _max_value)
{
    const auto max_value = static_cast<int32_t>(std::abs(_max_value) + 0.5);
    if (_max_value < 0)
        return Int(-max_value, max_value);
    return Int(0, max_value);
}

int32_t Int(const int32_t _min_value, const int32_t _max_value)
{
    const auto max_value = std::max(_min_value, _max_value);
    if (max_value == _min_value)
        return _min_value;
    return std::uniform_int_distribution<int32_t>(_min_value, max_value)(Engine());
}

bool Bool(const double _probability)
{
    if (_probability <= 0.0)
        return false;
    if (_probability >= 1.0)
        return true;
    return Value() < _probability;
}

std::string UniqueString(const uint16_t _len, const char* _chars)
{
    const char* chars = (_chars && _chars[0]) ? _chars : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const auto  size  = static_cast<int32_t>(std::char_traits<char>::length(chars) - 1);

    std::string result;
    result.reserve(_len);
    for (uint16_t i = 0; i < _len; ++i)
        result.push_back(chars[Int(0, size)]);

    return result;
}

uint64_t UniqueUint64(const bool _avoid_high_bit)
{
    const auto value = Engine()();
    return _avoid_high_bit ? (value & UINT64_C(0x7FFFFFFFFFFFFFFF)) : value;
}

} // namespace xsdk::xbase::random
