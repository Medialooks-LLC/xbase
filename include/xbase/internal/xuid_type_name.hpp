#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace xsdk::xbase::impl {

/**
 * @brief Check whether a character is treated as whitespace.
 * @param _ch Character to inspect.
 * @return `true` if the character is whitespace, otherwise `false`.
 */
constexpr bool IsSpace(const char _ch) noexcept
{
    return _ch == ' ' || _ch == '\t' || _ch == '\n' || _ch == '\r' || _ch == '\f' || _ch == '\v';
}

/**
 * @brief Remove leading and trailing whitespace from a string view.
 * @param _str Input string view.
 * @return Trimmed string view.
 */
constexpr std::string_view Trim(std::string_view _str) noexcept
{
    while (!_str.empty() && IsSpace(_str.front()))
        _str.remove_prefix(1);

    while (!_str.empty() && IsSpace(_str.back()))
        _str.remove_suffix(1);

    return _str;
}

/**
 * @brief Check whether a string starts with a given prefix.
 * @param _str Input string view.
 * @param _prefix Prefix to match.
 * @return `true` if `_str` starts with `_prefix`, otherwise `false`.
 */
constexpr bool StartsWith(const std::string_view _str, const std::string_view _prefix) noexcept
{
    return _str.size() >= _prefix.size() && _str.substr(0, _prefix.size()) == _prefix;
}

/**
 * @brief Check whether a string ends with a given suffix.
 * @param _str Input string view.
 * @param _suffix Suffix to match.
 * @return `true` if `_str` ends with `_suffix`, otherwise `false`.
 */
constexpr bool EndsWith(const std::string_view _str, const std::string_view _suffix) noexcept
{
    return _str.size() >= _suffix.size() && _str.substr(_str.size() - _suffix.size()) == _suffix;
}

/**
 * @brief Remove leading C++ type-kind prefixes from a type name.
 * @param _str Raw type name.
 * @return Type name without leading `class`, `struct`, or `enum` prefixes.
 */
constexpr std::string_view RemoveLeadingTypePrefixes(std::string_view _str) noexcept
{
    constexpr std::array<std::string_view, 5> prefixes = {"enum class ", "enum struct ", "class ", "struct ", "enum "};

    _str = Trim(_str);

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto prefix : prefixes) {
            if (StartsWith(_str, prefix)) {
                _str    = Trim(_str.substr(prefix.size()));
                changed = true;
                break;
            }
        }
    }

    return _str;
}

/**
 * @brief Check whether a type spelling has top-level pointer, reference, or array decorators.
 * @param _str Type spelling to inspect.
 * @return `true` if a top-level decorator is present, otherwise `false`.
 */
constexpr bool HasTopLevelDecorator(const std::string_view _str) noexcept
{
    int angle_depth = 0;
    for (const char ch : _str) {
        if (ch == '<') {
            ++angle_depth;
            continue;
        }
        if (ch == '>') {
            if (angle_depth > 0)
                --angle_depth;
            continue;
        }

        if (angle_depth == 0 && (ch == '*' || ch == '&' || ch == '[' || ch == '('))
            return true;
    }

    return false;
}

/**
 * @brief Fixed-size buffer used to build a canonical type name at compile time.
 * @tparam N Buffer capacity in characters.
 */
template <size_t N>
struct CanonicalTypeNameBuffer {
    std::array<char, N> data {};
    size_t              size      = 0;
    bool                truncated = false;
};

/**
 * @brief Append one character to the canonical name buffer.
 * @tparam N Buffer capacity in characters.
 * @param _buffer Destination buffer.
 * @param _ch Character to append.
 */
template <size_t N>
constexpr void AppendChar(CanonicalTypeNameBuffer<N>& _buffer, const char _ch) noexcept
{
    if (_buffer.size < _buffer.data.size()) {
        _buffer.data[_buffer.size++] = _ch;
        return;
    }

    _buffer.truncated = true;
}

/**
 * @brief Append a string view to the canonical name buffer.
 * @tparam N Buffer capacity in characters.
 * @param _buffer Destination buffer.
 * @param _str String view to append.
 */
template <size_t N>
constexpr void AppendString(CanonicalTypeNameBuffer<N>& _buffer, const std::string_view _str) noexcept
{
    for (const char ch : _str)
        AppendChar(_buffer, ch);
}

/**
 * @brief Append a string view while collapsing repeated whitespace.
 * @tparam N Buffer capacity in characters.
 * @param _buffer Destination buffer.
 * @param _str String view to append.
 */
template <size_t N>
constexpr void AppendCollapsed(CanonicalTypeNameBuffer<N>& _buffer, const std::string_view _str) noexcept
{
    bool prev_space = false;
    for (const char ch : _str) {
        if (IsSpace(ch)) {
            if (!_buffer.size || prev_space)
                continue;

            AppendChar(_buffer, ' ');
            prev_space = true;
            continue;
        }

        AppendChar(_buffer, ch);
        prev_space = false;
    }

    if (_buffer.size && _buffer.data[_buffer.size - 1] == ' ')
        --_buffer.size;
}

/**
 * @brief Extract the compiler-provided raw spelling of a type name.
 * @tparam T Type whose raw name is requested.
 * @return Raw compiler-specific type spelling.
 */
template <class T>
constexpr std::string_view TypeNameRaw() noexcept
{
#if defined(__clang__)
    // std::string_view __cdecl xsdk::xbase::impl::TypeNameRaw(void) [T = TestClass<int>]  clang-cl/LLVM
    // "std::string_view xsdk::xbase::impl::TypeNameRaw() [T = xtest::TestClass2]" AppleClang
    // "constexpr std::string_view xsdk::xbase::impl::TypeNameRaw() [with T = TestClass<long int>;
    // std::string_view = std::basic_string_view<char>]" GCC-compatible format
    constexpr std::string_view name_str = __PRETTY_FUNCTION__;
    constexpr std::string_view prefix   = "T = ";
    constexpr auto             pos      = name_str.find(prefix);

    if constexpr (pos == std::string_view::npos)
        return name_str;

    constexpr auto             wo_prefix      = name_str.substr(pos + prefix.size());
    constexpr std::string_view end_name_chars = "];";
    return wo_prefix.substr(0, wo_prefix.find_first_of(end_name_chars));
#elif defined(_MSC_VER)
    // class std::basic_string_view<char,struct std::char_traits<char> > __cdecl
    // xsdk::xbase::impl::TypeNameRaw<class TestClass<long>>(void) noexcept
    constexpr std::string_view name_str = __FUNCSIG__;
    constexpr std::string_view prefix   = "xsdk::xbase::impl::TypeNameRaw<";
    constexpr std::string_view postfix  = ">(void) noexcept";

    constexpr auto pos = name_str.find(prefix);
    if constexpr (pos == std::string_view::npos)
        return name_str;

    constexpr auto raw_name = name_str.substr(pos + prefix.size());
    return raw_name.substr(0, raw_name.rfind(postfix));
#else
    // "constexpr std::string_view xsdk::xbase::impl::TypeNameRaw() [with T = TestClass<long int>;
    // std::string_view = std::basic_string_view<char>]" GCC Linux
    constexpr std::string_view name_str = __PRETTY_FUNCTION__;
    constexpr std::string_view prefix   = "T = ";
    constexpr auto             pos      = name_str.find(prefix);

    if constexpr (pos == std::string_view::npos)
        return name_str;

    constexpr auto             wo_prefix      = name_str.substr(pos + prefix.size());
    constexpr std::string_view end_name_chars = "];";
    return wo_prefix.substr(0, wo_prefix.find_first_of(end_name_chars));
#endif
}

/**
 * @brief Build a canonical type name from a raw compiler spelling.
 * @tparam N Buffer capacity in characters.
 * @param _raw_name Raw compiler-specific type spelling.
 * @return Canonicalized type name buffer.
 */
template <size_t N>
constexpr CanonicalTypeNameBuffer<N> CanonicalizeTypeName(std::string_view _raw_name) noexcept
{
    CanonicalTypeNameBuffer<N> buffer {};

    auto name = Trim(_raw_name);
    name      = RemoveLeadingTypePrefixes(name);

    bool       const_qualified   = false;
    const bool simple_named_type = !HasTopLevelDecorator(name);

    if (simple_named_type && StartsWith(name, "const ")) {
        const_qualified = true;
        name            = Trim(name.substr(std::string_view("const ").size()));
        name            = RemoveLeadingTypePrefixes(name);
    }

    if (simple_named_type && EndsWith(name, " const")) {
        const_qualified = true;
        name            = Trim(name.substr(0, name.size() - std::string_view(" const").size()));
    }

    name = RemoveLeadingTypePrefixes(name);

    if (const_qualified)
        AppendString(buffer, "const ");

    AppendCollapsed(buffer, name);
    return buffer;
}

/**
 * @brief Compile-time storage for the canonical name of a type.
 * @tparam T Type whose canonical name is stored.
 */
template <class T>
struct CanonicalTypeNameStorage {
    static constexpr size_t kExtraCapacity   = 48;
    static constexpr auto   canonical_buffer = CanonicalizeTypeName<TypeNameRaw<T>().size() + kExtraCapacity>(
        TypeNameRaw<T>());
    static_assert(!canonical_buffer.truncated, "Canonical type name buffer is too small. Increase kExtraCapacity.");
    static constexpr std::string_view value {canonical_buffer.data.data(), canonical_buffer.size};
};

} // namespace xsdk::xbase::impl
