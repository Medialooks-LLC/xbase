#include "xbase/numbers.hpp"

#include <atomic>
#include <cmath>
#include <cstring>

namespace xsdk::xbase::numbers {

std::pair<int64_t, double> SplitDouble(const double _value)
{
    const auto int_value = std::llround(_value);
    return {int_value, _value - int_value};
}

std::pair<Rational, int64_t> Reduce(const int64_t _first, const int64_t _second)
{
    if (!_second || !_first)
        return {{static_cast<int32_t>(_first), static_cast<int32_t>(_second)}, 0};

    auto divisor = std::gcd(_first, _second);
    if (std::abs(_first / divisor) < std::numeric_limits<int32_t>::max() &&
        std::abs(_second / divisor) < std::numeric_limits<int32_t>::max()) {
        return {{static_cast<int32_t>(_first / divisor), static_cast<int32_t>(_second / divisor)}, divisor};
    }

    const double ratio = static_cast<double>(_first) / _second;
    return {{std::lround(1000 * ratio), 1000}, 0};
}

Rational DoubleToRational(const double _value, std::vector<Rational>&& _extra_check, const double _precision)
{
    constexpr double kIntegerPrecision = 1e-9;

    auto [int_value, int_frac] = SplitDouble(_value);
    if (std::abs(int_frac) < kIntegerPrecision)
        return Reduce(int_value, 1).first;

    static std::vector<Rational> well_known = {{1000, 1001},
                                               {4, 3},
                                               {16, 9},
                                               {3, 2},
                                               {5, 3},
                                               {11, 8},
                                               {14, 9},
                                               {37, 20},
                                               {256, 135},
                                               {11, 5},
                                               {64, 27},
                                               {47, 20},
                                               {239, 100},
                                               {12, 5},
                                               {69, 25},
                                               {32, 9},
                                               {18, 5}};

    auto* check_p = &well_known;
    if (!_extra_check.empty()) {
        _extra_check.insert(_extra_check.end(), well_known.begin(), well_known.end());
        check_p = &_extra_check;
    }

    for (auto rat : *check_p) {
        auto [int_val, frac] = SplitDouble(_value / RationalToDouble(rat));
        if (std::abs(frac) < _precision)
            return Reduce(rat.first * int_val, rat.second).first;
    }

    return Reduce(std::lround(1000.0 * _value), 1000).first;
}

int32_t ModSubMin(const uint32_t _first_by_mod, const uint32_t _second_by_mod, const uint32_t _modulo)
{
    int32_t result = static_cast<int32_t>(_first_by_mod) - static_cast<int32_t>(_second_by_mod);
    if (result >= static_cast<int32_t>(_modulo) / 2)
        result -= _modulo;
    else if (result <= -1 * static_cast<int32_t>(_modulo) / 2)
        result += _modulo;
    return result;
}

uint64_t ModIndex(const uint64_t _last, const uint64_t _value_by_mod, const uint32_t _modulo)
{
    const uint32_t last_by_mod = static_cast<uint32_t>(_last % _modulo);
    int32_t        sub         = ModSubMin(_value_by_mod % _modulo, last_by_mod, _modulo);
    if (sub + static_cast<int64_t>(_last) < 0)
        sub += _modulo;
    return _last + sub;
}

uint64_t ModDiv(const int64_t _value, const uint64_t _modulo)
{
    const auto modulo = static_cast<int64_t>(_modulo);
    const auto mod    = _value % modulo;
    return static_cast<uint64_t>(mod >= 0 ? mod : mod + modulo);
}

uint32_t ModOneAdd(const uint32_t _value, const int32_t _add, const uint32_t _modulo)
{
    return static_cast<uint32_t>(ModDiv(static_cast<int64_t>(_value) + _add - 1, _modulo)) + 1;
}

uint64_t NextUint64()
{
    static std::atomic<uint64_t> uid_gen = {1};
    return uid_gen.fetch_add(1);
}

uint32_t HashUint32(uint32_t _value)
{
    _value = ((_value >> 16) ^ _value) * 0x45d9f3b;
    _value = ((_value >> 16) ^ _value) * 0x45d9f3b;
    _value = (_value >> 16) ^ _value;
    return _value;
}

uint64_t HashUint64(uint64_t _value)
{
    _value = (_value ^ (_value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    _value = (_value ^ (_value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return _value ^ (_value >> 31);
}

uint64_t HashData(const size_t _size, const void* const _data)
{
    uint32_t seed = 0x56af121f;
    if (_size < sizeof(uint32_t)) {
        std::memcpy(&seed, _data, _size);
        return HashUint32(seed);
    }

    seed ^= (_size / sizeof(uint32_t));
    for (size_t i = 0; i < _size / sizeof(uint32_t); ++i) {
        auto value = HashUint32(static_cast<const uint32_t*>(_data)[i]);
        seed ^= value + 0x9e3779b9 + (value << 6) + (value >> 2);
    }

    if (_size % sizeof(uint32_t)) {
        auto value = HashUint32(*reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(_data) + _size -
                                                                   sizeof(uint32_t)));
        seed ^= value + 0x9e3779b9 + (value << 6) + (value >> 2);
    }

    return seed;
}

uint64_t HashString(const std::string_view _text)
{
    return _text.empty() ? 0 : HashData(_text.size(), _text.data());
}

} // namespace xsdk::xbase::numbers
