#include "clock_impl.h"

#include <chrono>
#include <cstdint>
#include <thread>

#include "../platform.h"
#ifdef _WIN32
    #include <sysinfoapi.h>
#else
    #include <sys/time.h>
#endif

namespace xsdk {

template <typename TSyncGenClock>
const xbase::ISyncGenerator::SPtrC& xclock::SyncGenStd(bool _monotonic_increase)
{
    static ISyncGenerator::SPtrC mono = std::make_shared<impl::SyncGeneratorStd<TSyncGenClock, time64::kSecond>>(true);
    static ISyncGenerator::SPtrC def  = std::make_shared<impl::SyncGeneratorStd<TSyncGenClock, time64::kSecond>>(false);
    return _monotonic_increase ? mono : def;
}
template const xbase::ISyncGenerator::SPtrC& xclock::SyncGenStd<std::chrono::steady_clock>(bool);
template const xbase::ISyncGenerator::SPtrC& xclock::SyncGenStd<std::chrono::system_clock>(bool);
#ifdef _WIN32
template const xbase::ISyncGenerator::SPtrC& xclock::SyncGenStd<std::chrono::high_resolution_clock>(bool);
#endif

xbase::IClock::UPtr xclock::Create(const ISyncGenerator::SPtrC& _sync_gen, std::optional<Time64>&& _start_time)
{
    assert(_sync_gen);
    if (!_sync_gen)
        return nullptr;

    return std::make_unique<impl::ClockBasic>(_sync_gen, std::move(_start_time));
}

xbase::IClock::UPtr xclock::Create(const Time64 _start_from, const bool _monotonic)
{
    return xclock::Create(xclock::SteadySyncGen(_monotonic), _start_from);
}

xbase::IClock::UPtr xclock::Clone(const IClock* _base_p, const AdjustType _adjuct_type, std::optional<Time64>&& _value)
{
    assert(_base_p);
    if (!_base_p)
        return nullptr;

    if (!_value.has_value() || _adjuct_type == AdjustType::kClockOffset) {
        auto offset = _base_p->SyncOffset() + _value.value_or(0);
        return std::make_unique<impl::ClockBasic>(offset, _base_p->SyncGenerator());
    }

    return std::make_unique<impl::ClockBasic>(_base_p->SyncGenerator(), std::move(_value));
}

xbase::IClock::UPtr xclock::CreateOrClone(const IClock*                _base_p,
                                          const ISyncGenerator::SPtrC& _sync_gen_for_new_clock,
                                          std::optional<Time64>&&      _start_time)
{
    if (_base_p)
        return xclock::Clone(_base_p, AdjustType::kStartTime, std::move(_start_time));

    return xclock::Create(_sync_gen_for_new_clock, std::move(_start_time));
}

// Aliases
xbase::Time64 xclock::SysTime() { return Timestamp<std::chrono::system_clock, time64::kSecond>(); }
xbase::Time64 xclock::HighResTime() { return Timestamp<std::chrono::high_resolution_clock, time64::kSecond>(); }
xbase::Time64 xclock::UtcTime()
{
    return Timestamp<std::chrono::system_clock, time64::kSecond>() + time64::kEpochShift;
}

static xbase::Time64 application_start_utc = xclock::UtcTime();
xbase::Time64 xclock::ApplicationStartUtc() { return application_start_utc; }

const xbase::ISyncGenerator::SPtrC& xclock::SysSyncGen(bool _monotonic_increase)
{
    return xclock::SyncGenStd<std::chrono::system_clock>(_monotonic_increase);
}
const xbase::ISyncGenerator::SPtrC& xclock::HighResSyncGen(bool _monotonic_increase)
{
    return xclock::SyncGenStd<std::chrono::high_resolution_clock>(_monotonic_increase);
}
const xbase::ISyncGenerator::SPtrC& xclock::SteadySyncGen(bool _monotonic_increase)
{
    return xclock::SyncGenStd<std::chrono::steady_clock>(_monotonic_increase);
}
const xbase::IClock* xclock::SysClock(bool _monotonic_increase)
{
    static auto clock_sys   = Create(SysSyncGen(false));
    static auto clock_sys_m = Create(SysSyncGen(true));
    return _monotonic_increase ? clock_sys_m.get() : clock_sys.get();
}
const xbase::IClock* xclock::HighResClock(bool _monotonic_increase)
{
    static auto clock_hr   = Create(HighResSyncGen(false), SysTime());
    static auto clock_hr_m = Create(HighResSyncGen(true), SysTime());
    return _monotonic_increase ? clock_hr_m.get() : clock_hr.get();
}
const xbase::IClock* xclock::UtcClock(bool _monotonic_increase)
{
    static auto clock_utc   = std::make_unique<impl::ClockBasic>(-1 * time64::kEpochSysToFileClock, SysSyncGen(false));
    static auto clock_utc_m = std::make_unique<impl::ClockBasic>(-1 * time64::kEpochSysToFileClock, SysSyncGen(true));
    return _monotonic_increase ? clock_utc_m.get() : clock_utc.get();
}

// ClockHR impl
xbase::ClockHR::ClockHR(const xbase::Time64 _start_time, bool _monotonic_increase)
    : start_(xclock::HighResSyncGen(_monotonic_increase)->Timestamp() - _start_time),
      monotonic_increase_(_monotonic_increase)
{
    lap_start_ = start_;
}

xbase::Time64 xbase::ClockHR::Time() const { return Time_() - start_; }

xbase::Time64 xbase::ClockHR::ResetTime(const xbase::Time64 _start)
{
    auto time = Time();
    start_ += time - _start;
    return time;
}
double xbase::ClockHR::ResetTimeMsec(const double _start_msec)
{
    return time64::ToMsec(ResetTime(time64::FromMsec(_start_msec)));
}

xbase::Time64 xbase::ClockHR::Lap() const { return Time_() - lap_start_; }

std::pair<bool, xbase::Time64> xbase::ClockHR::ResetLap(const xbase::Time64 _min_lap)
{
    auto lap = Lap();
    if (lap < _min_lap)
        return {false, lap};

    lap_start_ += lap;
    return {true, lap};
}

std::pair<bool, double> xbase::ClockHR::ResetLapMsec(const double _min_lap_msec)
{
    auto [is_reset, lap_time] = ResetLap(time64::FromMsec(_min_lap_msec));
    return {is_reset, time64::ToMsec(lap_time)};
}

inline xbase::Time64 xbase::ClockHR::Time_() const { return xclock::HighResSyncGen(monotonic_increase_)->Timestamp(); }

// Clock & Syn gen impl
namespace xbase::impl {

    ClockBasic::ClockBasic(const ISyncGenerator::SPtrC& _base_clock, std::optional<Time64>&& _clock_start_time)
        : sync_gen_p_(_base_clock)
    {
        if (_clock_start_time.has_value())
            ClockBasic::ResetTime(_clock_start_time.value());
        else
            start_rt_.store(0);

        step_start_rt_.store(start_rt_.load());
    }

    ClockBasic::ClockBasic(const Time64 _sync_offset, const ISyncGenerator::SPtrC& _base_clock)
        : sync_gen_p_(_base_clock),
          start_rt_(-1 * _sync_offset),
          step_start_rt_(-1 * _sync_offset)
    {
    }

    Time64 ClockBasic::Time(const Time64 _elapsed_from_time) const
    {
        if (_elapsed_from_time == time64::kNoVal)
            return BaseTimestamp_();

        return (BaseTimestamp_() - start_rt_.load()) - _elapsed_from_time;
    }

    Time64 ClockBasic::ResetTime(const Time64 _start_from_time)
    {
        auto timestamp_rt = BaseTimestamp_();
        auto time_rt      = timestamp_rt - start_rt_.load();
        start_rt_.store(timestamp_rt - _start_from_time);
        return time_rt;
    }

    Time64 ClockBasic::Lap() const { return (BaseTimestamp_() - step_start_rt_.load()); }

    std::pair<bool, Time64> ClockBasic::ResetLap(const Time64 _min_lap_value, const Time64 _start_lap_value)
    {
        auto start_rt = step_start_rt_.load();
        while (true) {
            auto timestamp_rt = BaseTimestamp_();
            auto step_rt      = timestamp_rt - start_rt;
            if (step_rt < _min_lap_value)
                return {false, step_rt};

            if (step_start_rt_.compare_exchange_strong(start_rt, timestamp_rt - _start_lap_value))
                return {true, step_rt};
        }
        assert(false);
    }
} // namespace xbase::impl
} // namespace xsdk
