#pragma once

#include "xbase.h"

#include <chrono>
#include <cstdint>
#include <thread>

namespace xsdk::xbase::impl {

template <class TClock, uint64_t TTicksPerSecond = time64::kSecond>
class SyncGeneratorStd final: public xbase::ISyncGenerator {
    static_assert(TTicksPerSecond > 0);

    const std::unique_ptr<xbase::Monotonic64> monotonic_p_ = nullptr;

public:
    explicit SyncGeneratorStd(bool _monotonic_increase)
        : monotonic_p_(_monotonic_increase ? std::make_unique<Monotonic64>() : nullptr)
    {
    }

public:
    virtual int64_t TicksPerSeconds() const override { return TTicksPerSecond; }

    virtual int64_t Timestamp(const uint64_t _units_per_second) const override
    {
        if (_units_per_second == 0 || _units_per_second == TTicksPerSecond)
            return BaseTimestamp_();

        auto ts = BaseTimestamp_();
        // todo: make integer multiply like muldiv64 etc.
        return (int64_t)((double)ts / TTicksPerSecond * _units_per_second);
    }

    virtual bool IsMonotonicIncrease() const override { return monotonic_p_ ? true : false; }

private:
    Time64 BaseTimestamp_() const
    {
        auto ts = xclock::Timestamp<TClock, time64::kSecond>();
        return monotonic_p_ ? monotonic_p_->Next(ts) : ts;
    }
};

class ClockBasic final: public xbase::IClock {

public:
    explicit ClockBasic(const ISyncGenerator::SPtrC& _base_clock, std::optional<Time64>&& _clock_start_time = {});
    ClockBasic(const Time64 _sync_offset, const ISyncGenerator::SPtrC& _base_clock);

public:
    virtual ISyncGenerator::SPtrC   SyncGenerator() const override { return sync_gen_p_; }
    virtual Time64                  SyncOffset() const override { return -1 * start_rt_.load(); }
    virtual Time64                  Time(const Time64 _elapsed_from_time = 0) const override;
    virtual Time64                  ResetTime(const Time64 _start_from_time = 0) override;
    virtual Time64                  Lap() const override;
    virtual std::pair<bool, Time64> ResetLap(const Time64 _min_lap_value, const Time64 _start_lap_value) override;

private:
    Time64 BaseTimestamp_() const { return sync_gen_p_->Timestamp(time64::kSecond); }

private:
    const ISyncGenerator::SPtrC sync_gen_p_;
    std::atomic<Time64>         start_rt_      = {};
    std::atomic<Time64>         step_start_rt_ = {};
};

} // namespace xsdk::xbase::impl
