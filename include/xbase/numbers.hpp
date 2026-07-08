#pragma once

#include "xbase/symbols.h"

/**
 * @file numbers.hpp
 * @brief Numeric helpers for optional conversion, addition, clamping, and alignment.
 */

#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace xsdk::xbase {
class Rational: public std::pair<int32_t, int32_t> {
public:
    using pair::operator=;
    using pair::pair;

    explicit operator bool() const { return first != 0 && second != 0; }
    double ToDouble(const double _default = 0.0) const { return second != 0 ? static_cast<double>(first) / second :
                                                                       _default; }
};

namespace detail {

    /**
     * @brief True for arithmetic non-bool types supported by the helpers in this file.
     */
    template <typename TType>
    inline constexpr bool kSupportedNumberV = std::is_arithmetic_v<TType> &&
                                              !std::is_same_v<std::remove_cv_t<TType>, bool>;

    /**
     * @brief Returns true if the floating point value is NaN.
     */
    template <typename TType>
    constexpr bool IsNaN(const TType _value)
    {
        if constexpr (std::is_floating_point_v<TType>) {
            // NOLINTNEXTLINE(misc-redundant-expression)
            return _value != _value;
        }

        return false;
    }

    /**
     * @brief Adjusts a floating point value so that truncation performs round-half-away-from-zero.
     *
     * Example:
     * - 1.4 -> 1.9  -> trunc -> 1
     * - 1.5 -> 2.0  -> trunc -> 2
     * - -1.4 -> -1.9 -> trunc -> -1
     * - -1.5 -> -2.0 -> trunc -> -2
     */
    template <typename TFloat>
    constexpr TFloat RoundHalfAwayFromZeroAdjust(const TFloat _value)
    {
        static_assert(std::is_floating_point_v<TFloat>);

        return _value >= static_cast<TFloat>(0) ? _value + static_cast<TFloat>(0.5) : _value - static_cast<TFloat>(0.5);
    }

    /**
     * @brief Result used by Clamp() when the source value is NaN.
     *
     * Floating targets preserve NaN when available. Non-floating targets return zero.
     */
    template <typename TClamp>
    constexpr TClamp NaNClampResult()
    {
        if constexpr (std::is_floating_point_v<TClamp> && std::numeric_limits<TClamp>::has_quiet_NaN)
            return std::numeric_limits<TClamp>::quiet_NaN();

        return TClamp {};
    }

} // namespace detail

namespace numbers {

    template <class TClampFor, class TNumber>
    constexpr TClampFor ClampNumber(const TNumber& _value)
    {
        if constexpr (std::numeric_limits<TNumber>::lowest() == 0) {
            return static_cast<TClampFor>(std::min(_value, static_cast<TNumber>(std::numeric_limits<TClampFor>::max())));
        }
        else {
            return static_cast<TClampFor>(std::min(std::max(static_cast<TNumber>(std::numeric_limits<TClampFor>::lowest()),
                                                            _value),
                                                   static_cast<TNumber>(std::numeric_limits<TClampFor>::max())));
        }
    }

    template <>
    inline uint64_t ClampNumber(const int64_t& _value)
    {
        return static_cast<uint64_t>(std::max<int64_t>(0, _value));
    }

    inline bool RationalToBool(const Rational _rational) { return _rational.first != 0 && _rational.second != 0; }

    inline double RationalToDouble(const Rational _rational, const double _default = 0.0)
    {
        return RationalToBool(_rational) ? static_cast<double>(_rational.first) / _rational.second : _default;
    }

    XBASE_API std::pair<int64_t, double> SplitDouble(double _value);

    XBASE_API std::pair<Rational, int64_t> Reduce(int64_t _first, int64_t _second);

    XBASE_API Rational DoubleToRational(double                  _value,
                                        std::vector<Rational>&& _extra_check = {},
                                        double                  _precision   = 0.004);

    XBASE_API int32_t ModSubMin(uint32_t _first_by_mod, uint32_t _second_by_mod, uint32_t _modulo);

    XBASE_API uint64_t ModIndex(uint64_t _last, uint64_t _value_by_mod, uint32_t _modulo);

    XBASE_API uint64_t ModDiv(int64_t _value, uint64_t _modulo);

    XBASE_API uint32_t ModOneAdd(uint32_t _value, int32_t _add, uint32_t _modulo);

    XBASE_API uint64_t NextUint64();

    XBASE_API uint32_t HashUint32(uint32_t _value);

    XBASE_API uint64_t HashUint64(uint64_t _value);

    XBASE_API uint64_t HashData(size_t _size, const void* _data);

    XBASE_API uint64_t HashString(std::string_view _text);

} // namespace numbers

/**
 * @brief Converts an optional number to another numeric type.
 *
 * Conversion policy:
 * - empty input -> std::nullopt
 * - NaN source -> std::nullopt
 * - out-of-range source -> std::nullopt or clamp to the nearest target limit
 * - floating-to-integral conversion uses round-half-away-from-zero
 *
 * Notes:
 * - For finite integral-to-floating conversions, the function allows normal C++ narrowing semantics.
 * - For floating-to-floating conversions, finite values outside target finite limits are rejected or clamped.
 * - Infinities are treated as out-of-range values for finite-limit clamping.
 *
 * @tparam TNumberTo Target arithmetic non-bool type.
 * @tparam TNumberFrom Source arithmetic non-bool type.
 * @param _number_from Optional source value.
 * @param _clamp_to_limits If true, clamp overflow/underflow to target limits instead of returning std::nullopt.
 * @return Converted value, clamped value, or std::nullopt.
 */
template <typename TNumberTo, typename TNumberFrom>
constexpr std::optional<TNumberTo> OptionalConvert( // NOLINT(readability-function-cognitive-complexity)
    const std::optional<TNumberFrom> _number_from,
    const bool                       _clamp_to_limits)
{
    static_assert(detail::kSupportedNumberV<TNumberTo>, "OptionalConvert requires arithmetic non-bool target type");
    static_assert(detail::kSupportedNumberV<TNumberFrom>, "OptionalConvert requires arithmetic non-bool source type");

    if (!_number_from.has_value())
        return std::nullopt;

    const auto number_from = _number_from.value();

    if constexpr (std::is_integral_v<TNumberFrom> && std::is_integral_v<TNumberTo>) {
        if constexpr (std::is_signed_v<TNumberFrom> == std::is_signed_v<TNumberTo>) {
            if constexpr (sizeof(TNumberFrom) > sizeof(TNumberTo)) {
                const auto min_to = static_cast<TNumberFrom>(std::numeric_limits<TNumberTo>::lowest());
                const auto max_to = static_cast<TNumberFrom>(std::numeric_limits<TNumberTo>::max());

                if (number_from < min_to) {
                    if (_clamp_to_limits)
                        return std::numeric_limits<TNumberTo>::lowest();
                    return std::nullopt;
                }
                if (number_from > max_to) {
                    if (_clamp_to_limits)
                        return std::numeric_limits<TNumberTo>::max();
                    return std::nullopt;
                }
            }
        }
        else if constexpr (std::is_signed_v<TNumberFrom> && std::is_unsigned_v<TNumberTo>) {
            if (number_from < 0) {
                if (_clamp_to_limits)
                    return TNumberTo {0};
                return std::nullopt;
            }

            if constexpr (sizeof(TNumberFrom) > sizeof(TNumberTo)) {
                using TNumberFromUnsigned = std::make_unsigned_t<TNumberFrom>;

                const auto number_from_unsigned = static_cast<TNumberFromUnsigned>(number_from);
                const auto max_to = static_cast<TNumberFromUnsigned>(std::numeric_limits<TNumberTo>::max());

                if (number_from_unsigned > max_to) {
                    if (_clamp_to_limits)
                        return std::numeric_limits<TNumberTo>::max();
                    return std::nullopt;
                }
            }
        }
        else {
            if constexpr (sizeof(TNumberFrom) >= sizeof(TNumberTo)) {
                using TNumberToUnsigned = std::make_unsigned_t<TNumberTo>;

                const auto max_to = static_cast<TNumberFrom>(
                    static_cast<TNumberToUnsigned>(std::numeric_limits<TNumberTo>::max()));

                if (number_from > max_to) {
                    if (_clamp_to_limits)
                        return std::numeric_limits<TNumberTo>::max();
                    return std::nullopt;
                }
            }
        }

        return static_cast<TNumberTo>(number_from);
    }
    else if constexpr (std::is_floating_point_v<TNumberFrom> && std::is_integral_v<TNumberTo>) {
        if (detail::IsNaN(number_from))
            return std::nullopt;

        const auto number_ld = static_cast<long double>(number_from);

        if constexpr (std::is_unsigned_v<TNumberTo>) {
            const auto lower_exclusive = -0.5L;
            const auto upper_exclusive = static_cast<long double>(std::numeric_limits<TNumberTo>::max()) + 0.5L;

            if (!(number_ld > lower_exclusive)) {
                if (_clamp_to_limits)
                    return TNumberTo {0};
                return std::nullopt;
            }
            if (!(number_ld < upper_exclusive)) {
                if (_clamp_to_limits)
                    return std::numeric_limits<TNumberTo>::max();
                return std::nullopt;
            }

            if (number_from < static_cast<TNumberFrom>(0))
                return TNumberTo {0};

            return static_cast<TNumberTo>(detail::RoundHalfAwayFromZeroAdjust(number_from));
        }
        else {
            const auto lower_exclusive = static_cast<long double>(std::numeric_limits<TNumberTo>::lowest()) - 0.5L;
            const auto upper_exclusive = static_cast<long double>(std::numeric_limits<TNumberTo>::max()) + 0.5L;

            if (!(number_ld > lower_exclusive)) {
                if (_clamp_to_limits)
                    return std::numeric_limits<TNumberTo>::lowest();
                return std::nullopt;
            }
            if (!(number_ld < upper_exclusive)) {
                if (_clamp_to_limits)
                    return std::numeric_limits<TNumberTo>::max();
                return std::nullopt;
            }

            return static_cast<TNumberTo>(detail::RoundHalfAwayFromZeroAdjust(number_from));
        }
    }
    else if constexpr (std::is_integral_v<TNumberFrom> && std::is_floating_point_v<TNumberTo>) {
        return static_cast<TNumberTo>(number_from);
    }
    else {
        static_assert(std::is_floating_point_v<TNumberFrom> && std::is_floating_point_v<TNumberTo>);

        if (detail::IsNaN(number_from))
            return std::nullopt;

        const auto number_ld = static_cast<long double>(number_from);
        const auto min_to    = static_cast<long double>(std::numeric_limits<TNumberTo>::lowest());
        const auto max_to    = static_cast<long double>(std::numeric_limits<TNumberTo>::max());

        if (number_ld < min_to) {
            if (_clamp_to_limits)
                return std::numeric_limits<TNumberTo>::lowest();
            return std::nullopt;
        }
        if (number_ld > max_to) {
            if (_clamp_to_limits)
                return std::numeric_limits<TNumberTo>::max();
            return std::nullopt;
        }

        return static_cast<TNumberTo>(number_from);
    }
}

/**
 * @brief Adds a value to an optional number.
 *
 * Addition policy:
 * - empty input -> empty output
 * - NaN input or NaN input or NaN result -> std::nullopt
 * - zero addend -> unchanged input
 * - overflow/underflow -> std::nullopt or clamp to the nearest type limit
 *
 * @tparam TNumber Arithmetic non-bool type.
 * @tparam TAdd Arithmetic non-bool type for the value to add.
 * @param _number Optional input value.
 * @param _add_value Value to add.
 * @param _clamp_to_limits If true, clamp overflow/underflow instead of returning std::nullopt.
 * @return Updated value, clamped value, or std::nullopt.
 */
template <typename TNumber, typename TAdd>
constexpr std::optional<TNumber> OptionalAdd( // NOLINT(readability-function-cognitive-complexity)
    const std::optional<TNumber> _number,
    const TAdd                   _add_value,
    const bool                   _clamp_to_limits)
{
    static_assert(detail::kSupportedNumberV<TNumber>, "OptionalAdd requires arithmetic non-bool target type");
    static_assert(detail::kSupportedNumberV<TAdd>, "OptionalAdd requires arithmetic non-bool add type");

    if (!_number.has_value())
        return std::nullopt;

    const auto number = _number.value();

    if (detail::IsNaN(number) || detail::IsNaN(_add_value))
        return std::nullopt;

    if (_add_value == 0)
        return _number;

    if constexpr (std::is_integral_v<TNumber> && std::is_integral_v<TAdd>) {
        using TNumberU = std::make_unsigned_t<TNumber>;
        using TAddU    = std::make_unsigned_t<TAdd>;
        using TCommonU = std::common_type_t<TNumberU, TAddU>;

        constexpr auto abs_to_unsigned = [](const auto _value) constexpr {
            using TValue  = decltype(_value);
            using TValueU = std::make_unsigned_t<TValue>;

            if constexpr (std::is_signed_v<TValue>) {
                return _value >= 0 ? static_cast<TValueU>(_value) : static_cast<TValueU>(-(_value + 1)) + 1;
            }
            else {
                return static_cast<TValueU>(_value);
            }
        };

        if constexpr (std::is_unsigned_v<TNumber>) {
            if constexpr (std::is_signed_v<TAdd>) {
                if (_add_value < 0) {
                    const auto sub      = static_cast<TCommonU>(abs_to_unsigned(_add_value));
                    const auto number_u = static_cast<TCommonU>(number);

                    if (number_u < sub) {
                        if (_clamp_to_limits)
                            return TNumber {0};
                        return std::nullopt;
                    }

                    return static_cast<TNumber>(number_u - sub);
                }
            }

            const auto add      = static_cast<TCommonU>(abs_to_unsigned(_add_value));
            const auto number_u = static_cast<TCommonU>(number);
            const auto max_u    = static_cast<TCommonU>(std::numeric_limits<TNumber>::max());

            if (number_u > max_u - add) {
                if (_clamp_to_limits)
                    return std::numeric_limits<TNumber>::max();
                return std::nullopt;
            }

            return static_cast<TNumber>(number_u + add);
        }
        else {
            constexpr auto signed_from_magnitude = [abs_to_unsigned](const auto _magnitude) constexpr -> TNumber {
                using TMagnitude = decltype(_magnitude);
                using TNumberUU  = std::make_unsigned_t<TNumber>;

                const auto min_mag = static_cast<TMagnitude>(abs_to_unsigned(std::numeric_limits<TNumber>::lowest()));

                if (_magnitude == 0)
                    return TNumber {0};

                if (_magnitude == min_mag)
                    return std::numeric_limits<TNumber>::lowest();

                return static_cast<TNumber>(-static_cast<TNumber>(static_cast<TNumberUU>(_magnitude)));
            };

            const auto min_mag = static_cast<TCommonU>(abs_to_unsigned(std::numeric_limits<TNumber>::lowest()));
            const auto max_u   = static_cast<TCommonU>(std::numeric_limits<TNumber>::max());

            if constexpr (std::is_signed_v<TAdd>) {
                if (_add_value < 0) {
                    const auto sub = static_cast<TCommonU>(abs_to_unsigned(_add_value));

                    if (number >= 0) {
                        const auto number_u = static_cast<TCommonU>(static_cast<TNumberU>(number));

                        if (sub <= number_u)
                            return static_cast<TNumber>(number_u - sub);

                        const auto neg_mag = static_cast<TCommonU>(sub - number_u);
                        if (neg_mag > min_mag) {
                            if (_clamp_to_limits)
                                return std::numeric_limits<TNumber>::lowest();
                            return std::nullopt;
                        }

                        return signed_from_magnitude(neg_mag);
                    }

                    const auto number_mag = static_cast<TCommonU>(abs_to_unsigned(number));
                    if (sub > min_mag - number_mag) {
                        if (_clamp_to_limits)
                            return std::numeric_limits<TNumber>::lowest();
                        return std::nullopt;
                    }

                    return signed_from_magnitude(static_cast<TCommonU>(number_mag + sub));
                }
            }

            const auto add = static_cast<TCommonU>(abs_to_unsigned(_add_value));

            if (number >= 0) {
                const auto number_u = static_cast<TCommonU>(static_cast<TNumberU>(number));

                if (add > max_u - number_u) {
                    if (_clamp_to_limits)
                        return std::numeric_limits<TNumber>::max();
                    return std::nullopt;
                }

                return static_cast<TNumber>(number_u + add);
            }

            const auto number_mag = static_cast<TCommonU>(abs_to_unsigned(number));
            if (add < number_mag)
                return signed_from_magnitude(static_cast<TCommonU>(number_mag - add));

            const auto pos = static_cast<TCommonU>(add - number_mag);
            if (pos > max_u) {
                if (_clamp_to_limits)
                    return std::numeric_limits<TNumber>::max();
                return std::nullopt;
            }

            return static_cast<TNumber>(pos);
        }
    }
    else {
        const auto result = static_cast<long double>(number) + static_cast<long double>(_add_value);
        return OptionalConvert<TNumber>(std::optional<long double> {result}, _clamp_to_limits);
    }
}

/**
 * @brief Clamps a numeric value to the range of another numeric type.
 *
 * Behavior:
 * - finite out-of-range values are clamped to target limits
 * - NaN is preserved for floating targets when quiet_NaN is available
 * - NaN becomes zero for non-floating targets
 * - floating-to-integral conversion uses round-half-away-from-zero
 *
 * @tparam TClamp Target arithmetic non-bool type.
 * @tparam TNumber Source arithmetic non-bool type.
 * @param _number Input value.
 * @return Clamped or converted result.
 */
template <typename TClamp,
          typename TNumber,
          std::enable_if_t<detail::kSupportedNumberV<TClamp> && detail::kSupportedNumberV<TNumber>, bool> = true>
constexpr TClamp Clamp(const TNumber _number)
{
    const auto converted = OptionalConvert<TClamp>(std::optional<TNumber> {_number}, true);
    if (converted.has_value())
        return converted.value();

    return detail::NaNClampResult<TClamp>();
}

/**
 * @brief Aligns a non-negative integer up to the next multiple of @_align.
 *
 * Safe behavior:
 * - if @_align is zero, returns the input unchanged
 * - for signed types, negative @_align or negative @_number returns the input unchanged
 * - on positive overflow, clamps to std::numeric_limits<TInteger>::max()
 *
 * @tparam TInteger Integral non-bool type.
 * @param _number Value to align.
 * @param _align Alignment step.
 * @return Aligned value or unchanged input for unsupported runtime preconditions.
 */
template <
    typename TInteger,
    std::enable_if_t<std::is_integral_v<TInteger> && !std::is_same_v<std::remove_cv_t<TInteger>, bool>, bool> = true>
constexpr TInteger AlignUp(const TInteger _number, const TInteger _align)
{
    if (_align == 0)
        return _number;

    if constexpr (std::is_signed_v<TInteger>) {
        if (_align < 0 || _number < 0)
            return _number;
    }

    const auto remainder = static_cast<TInteger>(_number % _align);
    if (remainder == 0)
        return _number;

    const auto add = static_cast<TInteger>(_align - remainder);
    if (_number > std::numeric_limits<TInteger>::max() - add)
        return std::numeric_limits<TInteger>::max();

    return static_cast<TInteger>(_number + add);
}

/**
 * @brief Aligns a non-negative integer down to the previous multiple of @_align.
 *
 * Safe behavior:
 * - if @_align is zero, returns the input unchanged
 * - for signed types, negative @_align or negative @_number returns the input unchanged
 *
 * @tparam TInteger Integral non-bool type.
 * @param _number Value to align.
 * @param _align Alignment step.
 * @return Aligned value or unchanged input for unsupported runtime preconditions.
 */
template <
    typename TInteger,
    std::enable_if_t<std::is_integral_v<TInteger> && !std::is_same_v<std::remove_cv_t<TInteger>, bool>, bool> = true>
constexpr TInteger AlignDown(const TInteger _number, const TInteger _align)
{
    if (_align == 0)
        return _number;

    if constexpr (std::is_signed_v<TInteger>) {
        if (_align < 0 || _number < 0)
            return _number;
    }

    const auto remainder = static_cast<TInteger>(_number % _align);
    return static_cast<TInteger>(_number - remainder);
}

} // namespace xsdk::xbase
