#include "xbase/xtime.h"

#include "xbase/strings.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <regex>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/time.h>
#endif

namespace xsdk::time64 {

time_t SysClockTime(int64_t* const _second_fraction_rt_p)
{
    auto timepoint = std::chrono::system_clock::now();
    if (_second_fraction_rt_p) {
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(timepoint.time_since_epoch()).count() %
                  1'000'000'000;
        *_second_fraction_rt_p = ns / 100;
    }
    return std::chrono::system_clock::to_time_t(timepoint);
}

int64_t UtcTime()
{
#ifdef _WIN32
    LONGLONG result = 0;
    GetSystemTimeAsFileTime(reinterpret_cast<FILETIME*>(&result));
    return result;
#else
    timeval tv = {};
    gettimeofday(&tv, nullptr);
    return (kEpochShiftSec + tv.tv_sec) * kSecond + tv.tv_usec * 10;
#endif
}

std::tm LocalTime(const time_t& _time)
{
    std::tm result = {};
#ifdef _WIN32
    ::localtime_s(&result, &_time);
#else
    ::localtime_r(&_time, &result);
#endif
    return result;
}

std::tm GmTime(const time_t& _time)
{
    std::tm result = {};
#ifdef _WIN32
    ::gmtime_s(&result, &_time);
#else
    ::gmtime_r(&_time, &result);
#endif
    return result;
}

std::tm SysClockTm(const bool _utc_time, int64_t _offset_msec, int64_t* const _second_fraction_rt_p)
{
    time_t now = SysClockTime(_second_fraction_rt_p);
    if (_second_fraction_rt_p && _offset_msec != 0) {
        *_second_fraction_rt_p += (_offset_msec % 1000) * kMsec;
        _offset_msec = (*_second_fraction_rt_p / kSecond) * 1000;
        *_second_fraction_rt_p %= kSecond;
    }

    now += std::max<int64_t>(-1 * now, _offset_msec / 1000);
    return _utc_time ? GmTime(now) : LocalTime(now);
}

std::string TimeNowString(const bool _utc_time, const bool _include_msec, const int64_t _offset_msec)
{
    int64_t fraction = 0;
    auto    tm       = SysClockTm(_utc_time, _offset_msec, _include_msec ? &fraction : nullptr);
    if (_include_msec)
        return xbase::strings::Format("%02d:%02d:%02d.%03d",
                                      tm.tm_hour,
                                      tm.tm_min,
                                      tm.tm_sec,
                                      static_cast<int32_t>(fraction / kMsec));
    return xbase::strings::Format("%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
}

std::tm StringToTime(const std::string& _time, uint8_t* const _succeeded_p)
{
    uint8_t succeeded = 0;
    std::tm result = {};
    result.tm_isdst = -1;
    std::regex date_time_regex(
        R"((20\d{2})[-.\\/](0[1-9]|1[0-2])[-.\\/](0[1-9]|1[0-9]|2[0-9]|3[0-1]).?(([01]?[0-9]|2[0-3]):([0-5]\d))?(:([0-5]\d))?)");
    std::smatch match;
    if (std::regex_match(_time, match, date_time_regex)) {
        result.tm_year = std::atoi(match[1].str().c_str()) - 1900;
        result.tm_mon  = std::atoi(match[2].str().c_str()) - 1;
        result.tm_mday = std::atoi(match[3].str().c_str());

        succeeded = 1;
        if (match[4].matched) {
            result.tm_hour = std::atoi(match[5].str().c_str());
            result.tm_min  = std::atoi(match[6].str().c_str());
            succeeded |= 2;
        }
        if (match[8].matched) {
            result.tm_sec = std::atoi(match[8].str().c_str());
            succeeded |= 4;
        }
    }

    if (_succeeded_p)
        *_succeeded_p = succeeded;
    return result;
}

std::time_t StringToTimeT(const std::string& _time, uint8_t* const _succeeded_p)
{
    uint8_t succeeded = 0;
    auto    timeinfo  = StringToTime(_time, &succeeded);
    if (_succeeded_p)
        *_succeeded_p = succeeded;
    return succeeded > 0 ? std::mktime(&timeinfo) : 0;
}

time_t CompilerDateToTime(const char* const _date)
{
    static const char kMonthNames[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    if (!_date || !_date[0])
        return 0;

    char        month[4]  = {};
    int         day       = 0;
    int         year      = 0;
    const char* month_pos = nullptr;
    if (std::sscanf(_date, "%3s %d %d", month, &day, &year) != 3 || year < 1900 ||
        (month_pos = std::strstr(kMonthNames, month)) == nullptr) {
        return 0;
    }

    std::tm result = {};
    result.tm_mon  = static_cast<int>((month_pos - kMonthNames) / 3);
    result.tm_mday = day;
    result.tm_year = year - 1900;
    result.tm_isdst = -1;
    return std::mktime(&result);
}

} // namespace xsdk::time64
