#pragma once

#include <any>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>

namespace xsdk {
namespace xbase {

    static constexpr auto npos {static_cast<size_t>(-1)};

    // 1 / 10'000'000 units (100 nsec, 1 / 10'000 msec)
    using Time64 = int64_t;

    // Move to something like numbers ?
    class Monotonic64 {
        std::atomic_int64_t value_ = std::numeric_limits<int64_t>::min();

    public:
        /**
         * @brief Increment the monotonic data value and ensure it remains monotonic.
         * @param _val The current data value.
         * @return The next monotonic data value.
         */
        int64_t Next(int64_t _val)
        {
            auto next_min = value_.fetch_add(1) + 1;
            auto val      = std::max(_val, next_min);
            while (next_min < _val) {
                if (value_.compare_exchange_strong(next_min, val))
                    break;
            }

            return val;
        }

        int64_t Reset(int64_t _min_val = std::numeric_limits<int64_t>::min()) { return value_.exchange(_min_val); }
        int64_t Value() const { return value_.load(); }
    };
    /**
     * @brief Monotonic increasing sequence generator template class.
     *
     * This template class Monotonic generates monotonically increasing sequence of values.
     * The template parameter T represents the data type of values in the sequence.
     * The second template parameter TIdx defaults to 0, it is used to provide a variant
     * of this template with different instance names.
     *
     * @tparam T Data type of values in the sequence.
     * @tparam TIdx Index of current template instance.
     */
    template <typename T, size_t TIdx = 0>
    class Monotonic {
    public:
        /**
         * @brief Increment the monotonic data value and ensure it remains monotonic.
         * @param _val The current data value.
         * @return The next monotonic data value.
         */
        static int64_t Next(int64_t _val)
        {
            static Monotonic64 mono64;
            return mono64.Next(_val);
        }
    };

} // namespace xbase

namespace time64 {

    using namespace xbase;

    static constexpr Time64 kMisec  = 10;
    static constexpr Time64 kMsec   = 10'000;
    static constexpr Time64 kSecond = 10'000'000;

    static constexpr Time64 kMinute = 60 * kSecond;
    static constexpr Time64 kHour   = 60 * kMinute;
    static constexpr Time64 kDay    = 24 * kHour;
    static constexpr Time64 kYear   = 365 * kDay;
    static constexpr Time64 kPast   = std::numeric_limits<Time64>::min() / 2;
    static constexpr Time64 kFuture = std::numeric_limits<Time64>::max() / 2;
    static constexpr Time64 kNoVal  = std::numeric_limits<Time64>::min();

    static constexpr Time64 kNsecPerUnit = 100;

    static constexpr Time64 kEpochShiftSec       = 11'644'473'600LL;
    static constexpr Time64 kEpochShift          = kEpochShiftSec * kSecond;
    static constexpr Time64 kEpochSysToFileClock = -1 * kEpochShift;
    static constexpr Time64 kEpochFileToSysClock = kEpochShift;

    constexpr double ToUnits(const Time64 _time_rt, const Time64 _unit)
    {
        return _time_rt != kNoVal ? (double)_time_rt / _unit : std::numeric_limits<double>::min();
    }
    constexpr Time64 FromUnits(const double _time_dbl, const Time64 _unit)
    {
        return _time_dbl != std::numeric_limits<double>::min() ? (Time64)(_time_dbl * _unit) : kNoVal;
    }

    constexpr double ToMsec(const Time64 _time_rt) { return ToUnits(_time_rt, kMsec); }
    constexpr Time64 FromMsec(const double& _time_dbl) { return FromUnits(_time_dbl, kMsec); }
    constexpr double ToSec(const Time64 _time_rt) { return ToUnits(_time_rt, kSecond); }
    constexpr Time64 FromSec(const double& _time_dbl) { return FromUnits(_time_dbl, kSecond); }

    inline std::optional<Time64> ToOptional(const Time64 _time_rt, const Time64 _invalid_rt_value = kNoVal)
    {
        return _time_rt != _invalid_rt_value ? std::optional<Time64>(_time_rt) : std::nullopt;
    }

} // namespace time64

} // namespace xsdk
