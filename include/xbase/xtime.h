#pragma once

#include <any>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <memory>
#include <numeric>
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

    constexpr bool IsValid(const Time64 _time_rt, const bool _inside_past_future = false)
    {
        return _time_rt != kNoVal && (!_inside_past_future || (_time_rt >= kPast && _time_rt <= kFuture));
    }

    constexpr bool IsValid(const std::optional<Time64> _time, const bool _inside_past_future = false)
    {
        return _time.has_value() && time64::IsValid(_time.value(), _inside_past_future);
    }

    // Convert time64 value to units in rational form
    constexpr double ToUnits(const Time64 _time_rt, const Time64 _unit_num_rt, const Time64 _unit_den_rt = 1)
    {
        return _time_rt != kNoVal && _unit_num_rt != 0 ? ((double)_time_rt / _unit_num_rt * _unit_den_rt) :
                                                         std::numeric_limits<double>::min();
    }

    // Convert units in rational form to time64
    constexpr Time64 FromUnits(const double _time_dbl, const Time64 _unit_num_rt, const Time64 _unit_den_rt = 1)
    {
        return _time_dbl != std::numeric_limits<double>::min() && _unit_den_rt != 0 ?
                   (Time64)(_time_dbl * _unit_num_rt / _unit_den_rt) :
                   kNoVal;
    }

    constexpr double ToMsec(const Time64 _time_rt) { return ToUnits(_time_rt, kMsec); }

    constexpr Time64 FromMsec(const double& _time_dbl) { return FromUnits(_time_dbl, kMsec); }

    constexpr double ToSec(const Time64 _time_rt) { return ToUnits(_time_rt, kSecond); }

    constexpr Time64 FromSec(const double& _time_dbl) { return FromUnits(_time_dbl, kSecond); }

    constexpr double ToTime(const Time64 _time_rt, const Time64 _unit) { return ToUnits(_time_rt, _unit); }

    constexpr Time64 FromTime(const double _time_dbl, const Time64 _unit) { return FromUnits(_time_dbl, _unit); }

    constexpr std::optional<double> ToSec(const std::optional<Time64> _time_rt)
    {
        return _time_rt.has_value() ? std::optional<double> {ToSec(_time_rt.value())} : std::nullopt;
    }

    constexpr std::optional<Time64> FromSec(const std::optional<double>& _time_dbl)
    {
        return _time_dbl.has_value() ? std::optional<Time64> {FromSec(_time_dbl.value())} : std::nullopt;
    }

    constexpr std::optional<Time64> ToOptional(const Time64 _time_rt, const Time64 _invalid_rt_value = kNoVal)
    {
        return _time_rt != _invalid_rt_value ? std::optional<Time64>(_time_rt) : std::nullopt;
    }

    template <typename TRep, typename TPeriod>
    xbase::Time64 FromDuration(const std::chrono::duration<TRep, TPeriod> _dur)
    {
        static constexpr int64_t nsec_per_tick = 1'000'000'000 / kSecond;
        return std::chrono::duration_cast<std::chrono::nanoseconds>(_dur).count() / nsec_per_tick;
    }

    time_t SysClockTime(int64_t* _second_fraction_rt_p = nullptr);

    int64_t UtcTime();

    std::tm LocalTime(const time_t& _time);

    std::tm GmTime(const time_t& _time);

    std::tm SysClockTm(bool _utc_time, int64_t _offset_msec = 0, int64_t* _second_fraction_rt_p = nullptr);

    std::string TimeNowString(bool _utc_time, bool _include_msec, int64_t _offset_msec = 0);

    std::tm StringToTime(const std::string& _time, uint8_t* _succeeded_p = nullptr);

    std::time_t StringToTimeT(const std::string& _time, uint8_t* _succeeded_p = nullptr);

    time_t CompilerDateToTime(const char* _date);

    constexpr Time64 BlockStart(const int64_t _idx,
                                const int64_t _block_len_num,
                                const int64_t _block_len_den = 1,
                                const Time64  _base          = 0)
    {
        assert(_block_len_num != 0 && _block_len_den != 0);
        const auto gcd = std::gcd(_block_len_num, _block_len_den);
        if (!gcd)
            return _idx;

        const auto num = _block_len_num / gcd;
        const auto den = _block_len_den / gcd;
        // TODO: MullDiv64
        return _base + ((_idx * num) + (den / 2)) / den;
    }

    enum class AlignType { kLower, kRound, kUpper };

    constexpr std::pair<Time64, int64_t> BlockAlign(const Time64    _val,
                                                    const int64_t   _block_len_num,
                                                    const int64_t   _block_len_den = 1,
                                                    const AlignType _align_type    = AlignType::kLower,
                                                    const Time64    _base          = 0)
    {
        assert(_block_len_num != 0 && _block_len_den != 0 && _block_len_num >= _block_len_den);
        if (_block_len_num == 0 || _block_len_den == 0 || _block_len_num < _block_len_den)
            return {_val, 0};

        const auto gcd = std::gcd(_block_len_num, _block_len_den);
        const auto num = _block_len_num / gcd;
        const auto den = _block_len_den / gcd;

        const auto num_rounding = _align_type == AlignType::kUpper ? (num - den) :
                                  _align_type == AlignType::kRound ? num / 2 :
                                                                     den - 1;
        const auto base_val     = _val - _base;
        const auto idx          = ((base_val * den) + num_rounding) / num;
#ifdef _DEBUG
        const auto aligned_val = BlockStart(idx, _block_len_num, _block_len_den);
        assert(idx < 0 || idx == ((aligned_val * den) + num_rounding) / num);
#endif
        return {BlockStart(idx, _block_len_num, _block_len_den, _base), idx};
    }

} // namespace time64

} // namespace xsdk
