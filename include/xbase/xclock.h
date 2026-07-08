#pragma once

#include "xbase/symbols.h"

#include "xpointers.hpp"
#include "xtime.h"
#include "xuid.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

namespace xsdk {

namespace xbase {

    /**
     * @brief ISyncGenerator - base class for xSDK clock objects.
     * Used for time syncronization between modules in pipelines.
     */
    class XBASE_API ISyncGenerator {
    public:
        virtual ~ISyncGenerator() = default;

        USING_PTRS(ISyncGenerator)

        /**
         * @brief Retrieves the number of ticks per second for this clock.
         * @return The number of ticks per second.
         */
        virtual int64_t TicksPerSeconds() const = 0;
        /**
         * @brief Retrieves the current timestamp in the format of number of units_per_second units.
         * @param _units_per_second Optional parameter specifying the unit of the timestamp. If 0, the default unit
         * is used.
         * @return The current timestamp.
         */
        virtual int64_t Timestamp(const uint64_t _units_per_second = 0) const = 0;
        /**
         * @brief Checks if the clock provides a monotonic increase in time.
         * @return True if the clock provides a monotonic increase in time, false otherwise.
         */
        virtual bool IsMonotonicIncrease() const = 0;
    };

    /**
     * @brief IClock - utility class over ISyncGenerator clock object.
     * Used for time syncronization between modules in pipelines.
     */
    class XBASE_API IClock {
    public:
        virtual ~IClock() = default;

        USING_PTRS(IClock)

    public:
        /**
         * @brief Get a pointer to the underlying sync generator.
         * @return Pointer to ISyncGenerator instance.
         */
        virtual ISyncGenerator::SPtrC SyncGenerator() const = 0;
        /**
         * @brief Get the clock offset from the underlying sync generator.
         * @return The offset value in 100 nanoseconds units.
         */
        virtual Time64 SyncOffset() const = 0;
        /**
         * @brief Get the current time from the underlying sync generator.
         * @param _elapsed_from_time Optional elapsed time from a given time since the last call to this method.
         * @return The current time in 100 nanoseconds units. If _elapsed_from_time is provided, the returned value is
         * the current time minus the elapsed time.
         * @note Time(time64::kNoVal) return uncorrected ISyncGenerator values (in 100 nsec units)
         */
        virtual Time64 Time(const Time64 _elapsed_from_time = 0) const = 0;

        /**
         * @brief Reset the clock to a new starting point and return the previous elapsed time.
         * @param _start_from_time New starting point for the clock in 100 nanoseconds units.
         * @return The elapsed time before the reset (in 100 nanoseconds units).
         */
        virtual Time64 ResetTime(const Time64 _start_from_time = 0) = 0;

        /**
         * @brief Get the time elapsed since the last call to ResetLap().
         * @return The elapsed time in 100 nanoseconds units.
         */
        virtual Time64 Lap() const = 0;
        /**
         * @brief Reset the lap timer and return the elapsed time and a boolean indicating if the reset was successful.
         * @param _min_lap_value   The minimum lap time to record.
         * @param _start_lap_value The starting point for the new lap timer in 100 nanoseconds units.
         * @return A pair containing a boolean value indicating if the reset was successful, and the elapsed time during
         * the current lap (in 100 nanoseconds units).
         */
        virtual std::pair<bool, Time64> ResetLap(const Time64 _min_lap_value   = 0,
                                                 const Time64 _start_lap_value = 0) = 0;
    };

    /**
     * @brief Vary basic class for measure times intervals based on High-Resolution SyncGenerator.
     */
    class XBASE_API ClockHR {
        Time64     start_;
        Time64     lap_start_;
        const bool monotonic_increase_;

    public:
        /**
         * @brief Constructor.
         * @param _start_time The start time, set to zero if not needed.
         * @param _monotonic_increase Whether the clock returns monotonic increasing times.
         */
        ClockHR(const Time64 _start_time = 0, bool _monotonic_increase = false);

        /**
         * @brief Get current time in 100 nanoseconds units.
         * @return The current time.
         */
        Time64 Time() const;

        /**
         * @brief Get current time in msec.
         * @return The current time in msec.
         */
        double TimeMsec() const { return time64::ToMsec(Time()); }

        /**
         * @brief Reset the clock to a new start time.
         * @param _start The new start time, set to zero if not needed.
         * @return The new start time.
         */
        Time64 ResetTime(const Time64 _start = 0);
        /**
         * @brief Reset the clock to a new start time in msec.
         * @param _start_msec The new start time in msec, set to zero if not needed.
         * @return The new start time in msec.
         */
        double ResetTimeMsec(const double _start_msec = 0);
        /**
         * @brief Get the time elapsed since the last ResetLap call in 100 nanoseconds units.
         * @return The time elapsed.
         */
        Time64 Lap() const;

        /**
         * @brief Get the time elapsed since the last ResetLap call in msec.
         * @return The time elapsed in msec.
         */
        double LapMsec() const { return time64::ToMsec(Lap()); }

        /**
         * @brief Reset the clock and start measuring a new interval.
         * @param _min_lap The minimum lap time, set to zero if need to reset lap immediatly.
         * @return A pair containing a boolean value indicating if the reset was successful, and the elapsed time during
         * the current lap in 100 nanoseconds units.
         */
        std::pair<bool, Time64> ResetLap(const Time64 _min_lap = 0);
        /**
         * @brief Reset the clock and start measuring a new interval in msec.
         * @param _min_lap_msec The minimum lap time, set to zero if need to reset lap immediatly.
         * @return A pair containing a boolean value indicating if the reset was successful, and the elapsed time during
         * the current lap in msec.
         */
        std::pair<bool, double> ResetLapMsec(const double _min_lap_msec = 0);

    private:
        Time64 Time_() const;
    };

    using ClockCpp = ClockHR;
} // namespace xbase

namespace xclock {
    using namespace xbase;

    /// @name xclock utilities
    /// @{
    /**
     * @brief Return timestamp from one of std clock in specified units
     * @tparam TClock The std::chrono clock to use to obtain timestamp
     * @tparam TTicksPerSecond Number of ticks in one second
     * @return An integer timestamp in nanoseconds
     */
    template <typename TClock, uint64_t TTicksPerSecond>
    static int64_t Timestamp()
    {
        static_assert(TTicksPerSecond > 0);
        static constexpr int64_t nsec_per_tick = 1'000'000'000 / TTicksPerSecond;
        return std::chrono::time_point_cast<std::chrono::nanoseconds>(TClock::now()).time_since_epoch().count() /
               nsec_per_tick;
    }

    /**
     * @brief helper for get clock time_point with msec from now
     */
    template <typename TClock>
    typename TClock::time_point TimepointFromNow(const double _add_msec)
    {
        return TClock::now() + std::chrono::microseconds((int64_t)std::ceil(_add_msec * 1000.0));
    }

    template <class TClock, class TDuration>
    double ElapcedMsec(const std::chrono::time_point<TClock, TDuration>& _abs_time)
    {
        auto microsec = std::chrono::duration_cast<std::chrono::microseconds>((TClock::now())-_abs_time);
        return microsec / 1000.0;
    }

    /**
     * @brief Return ISyncGenerator based on std::chrono library
     * @tparam TSyncGenClock The std::chrono clock to use as sync generator
     * @note Only the explicitly instantiated clocks exported by xbase are supported.
     */
    template <typename TSyncGenClock = std::chrono::steady_clock>
    const ISyncGenerator::SPtrC& SyncGenStd(bool _monotonic_increase);
    /// @}

    /**
     * @brief Create clock for specified sync generator
     * @param _sync_gen For which sync generator to create clock
     * @param _start_time Optional start time for the clock
     * @return A unique_ptr to the created clock
     */
    XBASE_API IClock::UPtr Create(const ISyncGenerator::SPtrC& _sync_gen, std::optional<Time64>&& _start_time = {});

    /**
     * @brief Create basic steady clock
     */
    XBASE_API IClock::UPtr Create(const Time64 _start_from = 0, const bool _monotonic = false);

    /**
     * @brief Adjustment types for IClock clone function
     * @see Clone
     */
    enum class AdjustType {
        /// @brief Clones the base clock with the given offset added to the cloned clock's time.
        kClockOffset,
        /// @brief Clones the base clock with the given time as the starting point for the cloned clock.
        kStartTime
    };
    /**
     * @brief Clones an existing IClock instance, optionally with a given offset or start time.
     * @param _base_p: The base clock from which to clone.
     * @param _adjuct_type: The type of adjustment to apply to the cloned clock. @see AdjustType
     * @param _value: The optional value to use for the adjustment.
     * @return A new instance of IClock, representing the cloned clock with the optional adjustment.
     */
    XBASE_API IClock::UPtr Clone(const IClock*           _base_p,
                                 const AdjustType        _adjuct_type = AdjustType::kClockOffset,
                                 std::optional<Time64>&& _value       = {});

    // Clone provides clock or create new one based on specified sync gen
    /**
     * @brief Clone an existing clock, or create a new one based on specified sync generator
     * @param _base_p The clock to clone or to base create new clock on
     * @param _sync_gen_for_new_clock The sync generator for the new clock
     * @param _start_time Optional start time for the new clock
     * @return A unique_ptr to the cloned or created clock
     */
    XBASE_API IClock::UPtr CreateOrClone(const IClock*                _base_p,
                                         const ISyncGenerator::SPtrC& _sync_gen_for_new_clock,
                                         std::optional<Time64>&&      _start_time = {});

    // Move to xbase ?

    /// @name Aliases for Timestamp<>
    /// @{
    /// @brief The current system time, in 100 nsec units since 1970 (windows filetime)
    XBASE_API Time64 SysTime();
    /// @brief The current high-resolution time, in seconds
    XBASE_API Time64 HighResTime();
    /**
     * @brief The current UTC time, in 100 nsec units since 1601 (unix timestamp)
     * @param _utc_timezone Timezone offset from UTC, in hours
     * @return The current UTC time
     */
    XBASE_API Time64 UtcTime();
    /**
     * @brief The application statrt UTC time, in 100 nsec units since 1601 (unix timestamp)
     * @param _utc_timezone Timezone offset from UTC, in hours
     * @return The current UTC time
     */
    XBASE_API Time64 ApplicationStartUtc();
    /// @}

    /// @name Aliases for SyncGenStd<>
    /// @{
    /**
     * @brief Get the sync generator based on system clock
     * @param _monotonic_increase Whether the sync generator provides monotonic time
     * @return The sync generator based on system clock
     */
    XBASE_API const ISyncGenerator::SPtrC& SysSyncGen(bool _monotonic_increase);
    /**
     * @brief Get the sync generator based on high-resolution clock
     * @param _monotonic_increase Whether the sync generator provides monotonic time
     * @return The sync generator based on high-resolution clock
     */
    XBASE_API const ISyncGenerator::SPtrC& HighResSyncGen(bool _monotonic_increase);
    /**
     * @brief Get the sync generator based on steady clock
     * @param _monotonic_increase Whether the sync generator provides monotonic time
     * @return The sync generator based on steady clock
     */
    XBASE_API const ISyncGenerator::SPtrC& SteadySyncGen(bool _monotonic_increase);
    /// @}

    /// @name Static clocks
    /// @{
    /**
     * @brief Get the system clock
     * @param _monotonic_increase Whether the clock provides monotonic time
     * @return The system clock
     */
    XBASE_API const IClock* SysClock(bool _monotonic_increase);
    /**
     * @brief Get the high-resolution clock
     * @param _monotonic_increase Whether the clock provides monotonic time
     * @return The high-resolution clock
     */
    XBASE_API const IClock* HighResClock(bool _monotonic_increase);
    /**
     * @brief Get the UTC clock
     * @param _monotonic_increase Whether the clock provides monotonic time
     * @return The UTC clock
     */
    XBASE_API const IClock* UtcClock(bool _monotonic_increase);
    /// @}

    /**
     * @brief helper for wait specified clock time
     * @return: Error ->   {kNoVal, kNoVal}
     *          No wait -> {0, 0}
     *          Wait ->    {The real wait time, the expected wait time, kNoVal if event signaled}
     */
    XBASE_API std::pair<xbase::Time64, xbase::Time64> WaitClockTime(const xbase::IClock* _clock_p,
                                                                    const xbase::Time64  _wait_until,
                                                                    const xbase::Time64  _skip_wait_if_less = 0);

    /**
     * @brief helper for wait condition_variable for specified clock time
     * @return: Error ->   {kNoVal, kNoVal}
     *          No wait -> {0, 0}
     *          Wait ->    {The real wait time, the expected wait time, kNoVal if event signaled}
     */
    XBASE_API std::pair<xbase::Time64, xbase::Time64> EventWaitClockTime(std::condition_variable&      _cv_event,
                                                                         std::unique_lock<std::mutex>* _lck_p,
                                                                         const xbase::IClock*          _clock_p,
                                                                         const xbase::Time64           _wait_until,
                                                                         const xbase::Time64 _skip_wait_if_less = 0);

#ifdef _MSC_VER
    #pragma warning(push)
    // Suppress C4251: private STL members don't need a DLL interface.
    #pragma warning(disable : 4251)
#endif

    /**
     * @brief helper class for decreaese wait time
     */
    class XBASE_API Countdown {
        std::atomic<xbase::Time64> end_time_ = {time64::kNoVal};

    public:
        Countdown(const Countdown& _copy) : end_time_(_copy.end_time_.load()) {}

        Countdown(Countdown&& _move) noexcept;
        Countdown(const std::optional<uint32_t> _wait_msec, const uint32_t _default_msec);

        Countdown(const double _wait_msec) { ResetToMsec(_wait_msec); }

    public:
        xbase::Time64 RemainingTime64() const { return end_time_.load() - xclock::HighResTime(); }

        uint32_t RemainingMsec() const;
        void     ResetToMsec(const double _remining_msec);
    };

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

} // namespace xclock

} // namespace xsdk
