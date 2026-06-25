#pragma once

/**
 * @file xenum.h
 * @brief Lightweight enum reflection helpers for string conversion and bitmask operations.
 *
 * The XENUM and XENUM_NESTED macros declare int32_t-based enum classes, while
 * XENUM64 and XENUM_NESTED64 declare int64_t-based enum classes. All macros
 * generate the reflection metadata required by the xsdk::xenum functions.
 *
 * Public API is intentionally limited to:
 * - XENUM / XENUM_CLASS / XENUM_NESTED declaration macros.
 * - XENUM64 / XENUM_CLASS64 / XENUM_NESTED64 declaration macros.
 * - XENUM_OPS / XENUM_NESTED_OPS bitmask operator macros.
 * - xsdk::xenum::ToString(), FromStringOne(), FromString(), HasFlag().
 * 
 * @note XENUM parser limitation:
 *       enumerator initializer expressions may contain commas only when they are
 *       protected by parentheses `()`, square brackets `[]`, braces `{}`, or quotes.
 *       Template argument lists with commas, such as `Value<int, long>::kValue`,
 *       are intentionally not parsed as a special case because angle brackets can
 *       conflict with shift-based flag expressions like `(1 << 12)`.
 *
 *       If a template expression with comma is needed, wrap it into a constexpr
 *       helper value and use that helper in the enum declaration.
 */

#include "xbase/strings.h"
#include "xbase/xuid.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace xsdk {

//-------------------------------- Public Interface --------------------------------

/**
 * @def XENUM_OPS
 * @brief Defines bitwise operators for a top-level or namespace enum class.
 *
 * Adds `&`, `|`, `^`, `~`, `&=` and `|=` overloads.
 * The operators return xsdk::xenum::Value so expressions such as `(flags & Flag::A)`
 * can be used directly in boolean contexts. Use xsdk::xenum::HasFlag() for explicit
 * flag checks.
 *
 * @param _enum_name Enum class name for which operators are generated.
 */
#define XENUM_OPS(_enum_name)                                                        \
    constexpr ::xsdk::xenum::Value<_enum_name> operator&(_enum_name l, _enum_name r) \
    {                                                                                \
        using U = std::underlying_type_t<_enum_name>;                                \
        return _enum_name(static_cast<U>(l) & static_cast<U>(r));                    \
    }                                                                                \
    constexpr ::xsdk::xenum::Value<_enum_name> operator|(_enum_name l, _enum_name r) \
    {                                                                                \
        using U = std::underlying_type_t<_enum_name>;                                \
        return _enum_name(static_cast<U>(l) | static_cast<U>(r));                    \
    }                                                                                \
    constexpr ::xsdk::xenum::Value<_enum_name> operator^(_enum_name l, _enum_name r) \
    {                                                                                \
        using U = std::underlying_type_t<_enum_name>;                                \
        return _enum_name(static_cast<U>(l) ^ static_cast<U>(r));                    \
    }                                                                                \
    constexpr _enum_name& operator&=(_enum_name& l, _enum_name r)                    \
    {                                                                                \
        using U  = std::underlying_type_t<_enum_name>;                               \
        return l = _enum_name(static_cast<U>(l) & static_cast<U>(r));                \
    }                                                                                \
    constexpr _enum_name& operator|=(_enum_name& l, _enum_name r)                    \
    {                                                                                \
        using U  = std::underlying_type_t<_enum_name>;                               \
        return l = _enum_name(static_cast<U>(l) | static_cast<U>(r));                \
    }                                                                                \
    constexpr ::xsdk::xenum::Value<_enum_name> operator~(_enum_name t)               \
    {                                                                                \
        using U = std::underlying_type_t<_enum_name>;                                \
        return _enum_name(~static_cast<U>(t));                                       \
    }

/**
 * @def XENUM_OPS32
 * @brief Backward-compatible alias for XENUM_OPS.
 */
#define XENUM_OPS32 XENUM_OPS

/**
 * @def XENUM_NESTED_OPS
 * @brief Defines bitwise operators for an enum class declared inside a class body.
 *
 * The generated operators are declared as `friend` functions, which allows ADL to
 * find them for nested enum types. Use xsdk::xenum::HasFlag() for explicit flag checks.
 *
 * @param _enum_name Nested enum class name for which operators are generated.
 */
#define XENUM_NESTED_OPS(_enum_name)                                                        \
    friend constexpr ::xsdk::xenum::Value<_enum_name> operator&(_enum_name l, _enum_name r) \
    {                                                                                       \
        using U = std::underlying_type_t<_enum_name>;                                       \
        return _enum_name(static_cast<U>(l) & static_cast<U>(r));                           \
    }                                                                                       \
    friend constexpr ::xsdk::xenum::Value<_enum_name> operator|(_enum_name l, _enum_name r) \
    {                                                                                       \
        using U = std::underlying_type_t<_enum_name>;                                       \
        return _enum_name(static_cast<U>(l) | static_cast<U>(r));                           \
    }                                                                                       \
    friend constexpr ::xsdk::xenum::Value<_enum_name> operator^(_enum_name l, _enum_name r) \
    {                                                                                       \
        using U = std::underlying_type_t<_enum_name>;                                       \
        return _enum_name(static_cast<U>(l) ^ static_cast<U>(r));                           \
    }                                                                                       \
    friend constexpr _enum_name& operator&=(_enum_name& l, _enum_name r)                    \
    {                                                                                       \
        using U  = std::underlying_type_t<_enum_name>;                                      \
        return l = _enum_name(static_cast<U>(l) & static_cast<U>(r));                       \
    }                                                                                       \
    friend constexpr _enum_name& operator|=(_enum_name& l, _enum_name r)                    \
    {                                                                                       \
        using U  = std::underlying_type_t<_enum_name>;                                      \
        return l = _enum_name(static_cast<U>(l) | static_cast<U>(r));                       \
    }                                                                                       \
    friend constexpr ::xsdk::xenum::Value<_enum_name> operator~(_enum_name t)               \
    {                                                                                       \
        using U = std::underlying_type_t<_enum_name>;                                       \
        return _enum_name(~static_cast<U>(t));                                              \
    }

/**
 * @def XENUM_NESTED_OPS32
 * @brief Backward-compatible alias for XENUM_NESTED_OPS.
 */
#define XENUM_NESTED_OPS32 XENUM_NESTED_OPS

/**
 * @def XENUM
 * @brief Declares a reflected enum class in namespace or global/class-external scope.
 *
 * Do not use inside a class body; use XENUM_NESTED for nested enum classes.
 *
 * @param _enum_name Enum class name.
 * @param ... Enumerator list, including optional explicit integer values.
 */
#define XENUM(_enum_name, ...)                                        \
    XENUM_DETAIL_MAKE(namespace, enum class, _enum_name, __VA_ARGS__) \
    XENUM_OPS(_enum_name)

/**
 * @def XENUM_CLASS
 * @brief Compatibility alias for XENUM.
 */
#define XENUM_CLASS XENUM

/**
 * @def XENUM64
 * @brief Declares a reflected int64_t-based enum class in namespace or global/class-external scope.
 *
 * Use this macro when enumerator values do not fit in int32_t, for example
 * 64-bit bitmasks. Do not use inside a class body; use XENUM_NESTED64 there.
 *
 * @param _enum_name Enum class name.
 * @param ... Enumerator list, including optional explicit integer values.
 */
#define XENUM64(_enum_name, ...)                                                    \
    XENUM_DETAIL_MAKE_TYPE(namespace, enum class, int64_t, _enum_name, __VA_ARGS__) \
    XENUM_OPS(_enum_name)

/**
 * @def XENUM_CLASS64
 * @brief Compatibility alias for XENUM64.
 */
#define XENUM_CLASS64 XENUM64

/**
 * @def XENUM_NESTED
 * @brief Declares a reflected enum class inside a class body.
 *
 * Example:
 * @code
 * struct IMediaStream {
 *     XENUM_NESTED(State, kStopped = 0, kStarted = 1)
 * };
 * @endcode
 *
 * @param _enum_name Nested enum class name.
 * @param ... Enumerator list, including optional explicit integer values.
 */
#define XENUM_NESTED(_enum_name, ...)                             \
    XENUM_DETAIL_MAKE(class, enum class, _enum_name, __VA_ARGS__) \
    XENUM_NESTED_OPS(_enum_name)

/**
 * @def XENUM_NESTED64
 * @brief Declares a reflected int64_t-based enum class inside a class body.
 *
 * Use this macro for nested enum classes with values that do not fit in int32_t,
 * for example 64-bit bitmasks.
 *
 * @param _enum_name Nested enum class name.
 * @param ... Enumerator list, including optional explicit integer values.
 */
#define XENUM_NESTED64(_enum_name, ...)                                         \
    XENUM_DETAIL_MAKE_TYPE(class, enum class, int64_t, _enum_name, __VA_ARGS__) \
    XENUM_NESTED_OPS(_enum_name)

namespace xenum_detail {

    /**
     * @brief Runtime reflection metadata for a single enum type.
     *
     * This is an implementation detail used by the generated detail_reflector_()
     * functions. It stores enum value/name pairs parsed from the macro argument list
     * and provides lookup methods used by xsdk::xenum conversion helpers.
     *
     * @note This class is intentionally not part of the public API.
     */
    class EnumReflection final {
    public:
        /**
         * @brief Builds reflection metadata from generated enum values and source text.
         *
         * @param values Array of enum underlying values stored as int64_t.
         * @param count Number of values in the array.
         * @param enum_name Unqualified enum type name.
         * @param enum_body Stringified enumerator list from the declaration macro.
         */
        EnumReflection(const int64_t* values, int32_t count, const char* enum_name, const char* enum_body);

        /**
         * @brief Destroys reflection metadata.
         */
        ~EnumReflection();

        EnumReflection(const EnumReflection&)                = delete;
        EnumReflection& operator=(const EnumReflection&)     = delete;
        EnumReflection(EnumReflection&&) noexcept            = delete;
        EnumReflection& operator=(EnumReflection&&) noexcept = delete;

        /**
         * @brief Returns the number of reflected enumerators.
         */
        [[nodiscard]] int32_t Count() const;

        /**
         * @brief Returns the unqualified enum type name.
         */
        [[nodiscard]] std::string_view EnumName() const;

        /**
         * @brief Finds an enumerator name by its underlying int64_t value.
         *
         * @param value Enum underlying value.
         * @return Enumerator name, or std::nullopt when value is unknown.
         */
        [[nodiscard]] std::optional<std::string_view> NameByValue(int64_t value) const;

        /**
         * @brief Finds an underlying int64_t value by enumerator name.
         *
         * @param name Enumerator name to search for.
         * @return Enum underlying value stored as int64_t, or std::nullopt when name is unknown.
         */
        [[nodiscard]] std::optional<int64_t> ValueByName(std::string_view name) const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

    /**
     * @brief Returns reflection metadata for an enum type.
     *
     * Uses ADL to find the detail_reflector_() function generated by XENUM or
     * XENUM_NESTED.
     *
     * @tparam EnumType Reflected enum class type.
     * @param value Optional value used only for ADL; defaults to a value-initialized enum.
     */
    template <typename EnumType>
    const EnumReflection& ReflectionFor(EnumType value = EnumType())
    {
        return detail_reflector_(value);
    }

    /**
     * @brief Checks whether a string view starts with a given prefix.
     */
    inline bool StartsWith(std::string_view value, std::string_view prefix)
    {
        return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
    }

} // namespace xenum_detail

namespace xenum {

    /**
     * @brief Checks whether all bits from `_flag` are set in `_enum`.
     *
     * A zero flag matches only a zero enum value.
     *
     * @tparam TEnum Enum class type.
     * @param _enum Value to inspect.
     * @param _flag Flag mask to check.
     */
    template <class TEnum>
    constexpr bool HasFlag(const TEnum& _enum, const TEnum& _flag)
    {
        using U               = std::underlying_type_t<TEnum>;
        const auto enum_value = static_cast<U>(_enum);
        const auto flag_value = static_cast<U>(_flag);

        if (flag_value == 0)
            return enum_value == 0;

        return (enum_value & flag_value) == flag_value;
    }

    /**
     * @brief Small wrapper returned by generated bitwise operators.
     *
     * The wrapper implicitly converts back to the enum type, while also allowing
     * explicit boolean checks for expressions such as `(flags & Flag::Enabled)`.
     *
     * @tparam T Enum class type.
     */
    template <typename T>
    struct Value {
        /** Wrapped enum value. */
        T t;

        /** Creates a wrapper from an enum value. */
        constexpr Value(T t) : t(t) {}

        /** Converts the wrapper back to the enum type. */
        constexpr operator T() const { return t; }

        /** Returns true when the wrapped enum value is non-zero. */
        constexpr explicit operator bool() const
        {
            using U = std::underlying_type_t<T>;
            return static_cast<U>(t) != 0;
        }
    };

    /**
     * @brief Converts an enum value to its reflected enumerator name.
     *
     * If the value is unknown, returns `_default` when it is not empty; otherwise
     * returns a fallback string in the form `TypeName(value)`.
     *
     * @tparam TEnum Reflected enum class type.
     * @param _enum_val Enum value to convert.
     * @param _default Optional fallback string for unknown values.
     * @param _removed_prefix Optional prefix to remove from a known enumerator name.
     * @return Enumerator name or fallback string.
     */
    template <class TEnum>
    std::string ToString(const TEnum&           _enum_val,
                         const std::string_view _default        = {},
                         const std::string_view _removed_prefix = {})
    {
        const auto int_val   = static_cast<int64_t>(static_cast<std::underlying_type_t<TEnum>>(_enum_val));
        const auto enum_name = xenum_detail::ReflectionFor<TEnum>().NameByValue(int_val);

        if (enum_name.has_value()) {
            if (!_removed_prefix.empty() && xenum_detail::StartsWith(*enum_name, _removed_prefix))
                return std::string(enum_name->substr(_removed_prefix.size()));

            return std::string(*enum_name);
        }

        if (!_default.empty())
            return std::string(_default);

        return std::string(xbase::TypeName<TEnum>()) + "(" + std::to_string(int_val) + ")";
    }

    /**
     * @brief Converts a single enumerator name to an enum value.
     *
     * The function also supports the project's `k` prefix convention: when the
     * provided name does not start with `k`, lookup is retried with `k` prepended.
     *
     * @tparam TEnum Reflected enum class type.
     * @param _str_value Enumerator name.
     * @param _default Value returned when conversion fails.
     * @return Converted enum value, or `_default`.
     */
    template <class TEnum>
    std::optional<TEnum> FromStringOne(const std::string_view _str_value, const std::optional<TEnum> _default = {})
    {
        if (_str_value.empty())
            return _default;

        const auto& reflection = xenum_detail::ReflectionFor<TEnum>();
        if (const auto value = reflection.ValueByName(_str_value); value.has_value())
            return static_cast<TEnum>(*value);

        // Special fix for 'k' prefix.
        if (_str_value.front() != 'k') {
            std::string prefixed_name;
            prefixed_name.reserve(_str_value.size() + 1);
            prefixed_name.push_back('k');
            prefixed_name.append(_str_value.data(), _str_value.size());

            if (const auto value = reflection.ValueByName(prefixed_name); value.has_value())
                return static_cast<TEnum>(*value);
        }

        return _default;
    }

    /**
     * @brief Converts a pipe-separated list of enumerator names to a combined enum value.
     *
     * Unknown tokens are ignored. If no token can be converted, `_default` is returned.
     *
     * @tparam TEnum Reflected enum class type.
     * @param _str_value String containing one or more names separated by `|`.
     * @param _default Value returned when conversion fails completely.
     * @return Combined enum value, or `_default`.
     */
    template <class TEnum>
    std::optional<TEnum> FromString(const std::string_view _str_value, const std::optional<TEnum> _default = {})
    {
        std::optional<TEnum> result;
        for (const auto token : xbase::strings::Split(_str_value, '|', true)) {
            auto enum_val = FromStringOne<TEnum>(token);
            if (enum_val.has_value())
                result = result.value_or(enum_val.value()) | enum_val.value();
        }

        return result.has_value() ? result : _default;
    }

    /**
     * @brief Converts a pipe-separated enum string and returns a plain enum value.
     *
     * @tparam TEnum Reflected enum class type.
     * @param _str_value String containing one or more names separated by `|`.
     * @param _default Value returned when conversion fails.
     * @return Converted enum value or `_default`.
     */
    template <class TEnum>
    inline TEnum FromString(const std::string_view _str_value, const TEnum& _default)
    {
        return FromString<TEnum>(_str_value).value_or(_default);
    }

} // namespace xenum

//----------------------------- Implementation Details -----------------------------

/**
 * @def XENUM_DETAIL_SPEC_namespace
 * @brief Generates a namespace-scope reflection function specifier.
 *
 * @internal
 */
#define XENUM_DETAIL_SPEC_namespace                                   \
    extern "C" { /* Protection from being used inside a class body */ \
    }                                                                 \
    inline
/**
 * @def XENUM_DETAIL_SPEC_class
 * @brief Generates a class-scope friend reflection function specifier.
 *
 * @internal
 */
#define XENUM_DETAIL_SPEC_class friend inline

/**
 * @def XENUM_DETAIL_STR
 * @brief Stringifies macro arguments.
 *
 * @internal
 */
#define XENUM_DETAIL_STR(x) #x

/**
 * @def XENUM_DETAIL_MAKE
 * @brief Declares an int32_t enum class and generates its reflection metadata provider.
 *
 * @internal
 */
#define XENUM_DETAIL_MAKE(_spec, _enum_decl, _enum_name, ...) \
    XENUM_DETAIL_MAKE_TYPE(_spec, _enum_decl, int32_t, _enum_name, __VA_ARGS__)

/**
 * @def XENUM_DETAIL_MAKE_TYPE
 * @brief Declares an enum class with the given underlying type and generates its reflection metadata provider.
 *
 * @internal
 */
#define XENUM_DETAIL_MAKE_TYPE(_spec, _enum_decl, _underlying_type, _enum_name, ...)                                  \
    _enum_decl                                                            _enum_name: _underlying_type {__VA_ARGS__}; \
    XENUM_DETAIL_SPEC_##_spec const ::xsdk::xenum_detail::EnumReflection& detail_reflector_(_enum_name)               \
    {                                                                                                                 \
        static const auto* reflector = [] {                                                                           \
            static _underlying_type detail_sval;                                                                      \
            detail_sval = 0;                                                                                          \
            struct DetailVal {                                                                                        \
                DetailVal(const DetailVal& rhs) : val_(rhs) { detail_sval = val_ + 1; }                               \
                DetailVal(_underlying_type val) : val_(val) { detail_sval = val_ + 1; }                               \
                DetailVal() : val_(detail_sval) { detail_sval = val_ + 1; }                                           \
                                                                                                                      \
                DetailVal&       operator=(const DetailVal&) { return *this; }                                        \
                DetailVal&       operator=(_underlying_type) { return *this; }                                        \
                                 operator _underlying_type() const { return val_; }                                   \
                _underlying_type val_;                                                                                \
            } __VA_ARGS__;                                                                                            \
            const int64_t detail_vals[] = {__VA_ARGS__};                                                              \
            return new ::xsdk::xenum_detail::EnumReflection(                                                          \
                detail_vals,                                                                                          \
                static_cast<int32_t>(sizeof(detail_vals) / sizeof(int64_t)),                                          \
                #_enum_name,                                                                                          \
                XENUM_DETAIL_STR((__VA_ARGS__)));                                                                     \
        }();                                                                                                          \
        return *reflector;                                                                                            \
    }

} // namespace xsdk
