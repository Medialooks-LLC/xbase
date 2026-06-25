#pragma once

#include "xbase/strings.h"
#include "xbase/xclock.h"
#include "xbase/xpointers.hpp"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

namespace xsdk::xbase::averages {

struct AverageInfo {
    double average          = 0.0;
    double stddev           = 0.0;
    double min              = 0.0;
    double max              = 0.0;
    double recent           = 0.0;
    size_t counters         = 0;
    size_t dev_min_counters = 0;
    size_t dev_max_counters = 0;

    std::string ToString(const double _scale = 1.0) const
    {
        return strings::Format("val:%.3f avg:%.3f(%zu) min:%.3f(%zu) max:%.3f(%zu) dev:%.3f",
                               recent / _scale,
                               average / _scale,
                               counters,
                               min / _scale,
                               dev_min_counters,
                               max / _scale,
                               dev_max_counters,
                               stddev / _scale);
    }
};

class SlidingAverage {
public:
    static constexpr size_t kDefaultWindowSize = 500;

    SlidingAverage(SlidingAverage&& _move) noexcept
        : counter_(_move.counter_.load()),
          average_(_move.average_.load()),
          recent_(_move.recent_),
          smoothing_(_move.smoothing_)
    {
    }

    explicit SlidingAverage(const size_t _window_size = kDefaultWindowSize) : counter_(0), average_(0.0)
    {
        smoothing_ = 2.0 / std::max<size_t>(2, _window_size + 1);
    }

    double AddValue(const double                _value,
                    const std::optional<double> _ratio_up   = std::nullopt,
                    const std::optional<double> _ratio_down = std::nullopt)
    {
        const double ratio_up   = _ratio_up.value_or(1.0);
        const double ratio_down = _ratio_down.value_or(ratio_up);
        double       average    = average_.load();
        double       smoothing  = smoothing_ * (_value >= average ? ratio_up : ratio_down);

        const uint64_t counter = counter_.fetch_add(1);
        if (counter < 2 || (ratio_up / ratio_down < 1000.0 && ratio_down / ratio_up < 1000.0))
            smoothing = std::max(2.0 / (1 + counter), smoothing);

        smoothing = std::min(1.0, smoothing);

        double next_average = 0.0;
        while (true) {
            next_average   = average * (1.0 - smoothing) + _value * smoothing;
            if (average_.compare_exchange_strong(average, next_average))
                break;
        }

        recent_ = _value;
        return next_average;
    }

    double Average(int64_t* _counter_p = nullptr, const double* _next_value_p = nullptr) const
    {
        if (_counter_p)
            *_counter_p = static_cast<int64_t>(counter_.load());

        if (_next_value_p)
            return average_.load() * (1.0 - smoothing_) + (*_next_value_p) * smoothing_;

        return average_.load();
    }

    double   Recent() const { return recent_; }
    double   DefaultWeight() const { return smoothing_; }
    uint64_t Counter() const { return counter_.load(); }

    bool Empty(uint64_t* _counter_p = nullptr) const
    {
        const uint64_t counter = counter_.load();
        if (_counter_p)
            *_counter_p = counter;
        return counter == 0;
    }

    void Reset(const size_t _window_size = 0, const double _set_value = 0.0)
    {
        if (_window_size > 0)
            smoothing_ = 2.0 / (_window_size + 1);

        average_.store(_set_value);
        counter_.store(0);
    }

    void SetValue(const double _set_value) { average_.store(_set_value); }

private:
    std::atomic<uint64_t> counter_;
    std::atomic<double>   average_;
    double                recent_    = 0.0;
    double                smoothing_ = 1.0;
};

class SlidingAverageEx: public SlidingAverage {
public:
    static constexpr size_t kMinMaxWindowSize = 5000;

    enum MinMaxCountType {
        kNoMinMaxCount       = 0,
        kCountAvgBased       = 1,
        kCountMinMaxBased    = 2,
        kCountAvgMinMaxBased = 3
    };

    SlidingAverageEx(SlidingAverageEx&&) noexcept = default;

    SlidingAverageEx(const size_t _min_max_window_size = kMinMaxWindowSize,
                     const size_t _avg_window_size     = kDefaultWindowSize)
        : SlidingAverage(_avg_window_size),
          avg_min_(_min_max_window_size),
          avg_max_(_min_max_window_size),
          min_max_smoothing_(2.0 / (2 + _min_max_window_size))
    {
        avg_min_.Reset(1);
        avg_max_.Reset(1);
        avg_deviation_.Reset(_avg_window_size);
    }

    double AddValue(const double                _value,
                    const double                _dev_min_max_scale = 2.0,
                    const MinMaxCountType       _count_type        = kCountAvgMinMaxBased,
                    const std::optional<double> _ratio_up          = std::nullopt,
                    const std::optional<double> _ratio_down        = std::nullopt)
    {
        auto avg_min = avg_min_.AddValue(_value, min_max_smoothing_, 1.0);
        auto avg_max = avg_max_.AddValue(_value, 1.0, min_max_smoothing_);

        const double avg_val = SlidingAverage::AddValue(_value, _ratio_up, _ratio_down);
        avg_deviation_.AddValue(std::pow(std::abs(_value - avg_val), 2));

        if (SlidingAverage::Counter() > 10 && _dev_min_max_scale > 0.0 && _count_type != kNoMinMaxCount) {
            double max_threshold = std::numeric_limits<double>::lowest();
            double min_threshold = std::numeric_limits<double>::max();
            if (_count_type & kCountAvgBased) {
                max_threshold = avg_val * _dev_min_max_scale;
                min_threshold = avg_val / _dev_min_max_scale;
            }
            if (_count_type & kCountMinMaxBased) {
                max_threshold = std::max(max_threshold, avg_max / _dev_min_max_scale);
                min_threshold = avg_min > 0 ? std::min(min_threshold, avg_min * _dev_min_max_scale) :
                                              avg_val / _dev_min_max_scale;
            }

            if (_value > max_threshold)
                ++dev_max_counters_;
            else if (_value < min_threshold)
                ++dev_min_counters_;
        }

        return avg_val;
    }

    AverageInfo Info() const
    {
        const auto dev = avg_deviation_.Average();
        const auto avg = SlidingAverage::Average();
        return {avg,
                dev > 0 ? std::sqrt(dev) : 0,
                avg_min_.Average(),
                avg_max_.Average(),
                SlidingAverage::Recent(),
                static_cast<size_t>(SlidingAverage::Counter()),
                dev_min_counters_,
                dev_max_counters_};
    }

    double MinValue() const { return avg_min_.Average(); }
    double MaxValue() const { return avg_max_.Average(); }
    double Jitter() const { return avg_deviation_.Average(); }

    void Reset(const size_t _min_max_window_size = 0, const size_t _avg_window_size = 0, const double _set_value = 0.0)
    {
        if (_min_max_window_size > 0)
            min_max_smoothing_ = 2.0 / (1 + _min_max_window_size);

        avg_deviation_.Reset(_avg_window_size);
        avg_min_.Reset();
        avg_max_.Reset();
        SlidingAverage::Reset(_avg_window_size, _set_value);
        dev_min_counters_ = 0;
        dev_max_counters_ = 0;
    }

private:
    SlidingAverage avg_min_;
    SlidingAverage avg_max_;
    SlidingAverage avg_deviation_;
    double         min_max_smoothing_ = 1.0;
    size_t         dev_min_counters_  = 0;
    size_t         dev_max_counters_  = 0;
};

class IntervalAverage: public SlidingAverageEx {
public:
    USING_PTRS(IntervalAverage)

    IntervalAverage(const size_t _min_max_window_size = kMinMaxWindowSize,
                    const size_t _avg_window_size     = kDefaultWindowSize)
        : SlidingAverageEx(_min_max_window_size, _avg_window_size),
          clock_(xclock::Create(xclock::HighResSyncGen(false)))
    {
    }

    double OnStart() { return time64::ToMsec(clock_->ResetLap().second); }

    double OnStop(double* _avg_p = nullptr, const double _dev_min_max_avg_scale = 2.0)
    {
        const double step = time64::ToMsec(clock_->ResetLap().second);
        if (Counter() == 1) {
            Reset();
            AddValue(step);
        }

        AddValue(step, _dev_min_max_avg_scale);
        if (_avg_p)
            *_avg_p = Average();

        return step;
    }

    double ElapsedMsec() const { return time64::ToMsec(clock_->Lap()); }

private:
    IClock::UPtr clock_;
};

} // namespace xsdk::xbase::averages
