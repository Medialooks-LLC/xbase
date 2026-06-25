#pragma once

#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <ios>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/// @file strings.h
/// @brief Lightweight ASCII-focused string utilities.
///
/// @details
/// - Most APIs operate on std::string_view to avoid extra allocations.
/// - Tokens returned by split functions always reference the original input.
/// - Case-insensitive operations are ASCII-only (no locale/Unicode).

namespace xsdk::xbase::strings {

/// @brief Default set of characters trimmed by Trim().
static constexpr std::string_view kDefaultTrimChars = " \r\n\t\v\f";

/// @name ASCII helpers
/// @{

/// @brief Converts an ASCII letter to lower case; leaves other characters unchanged.
/// @param _ch Input character.
/// @return Lowercased ASCII character or the original character.
constexpr char ToLower(char _ch) { return (_ch >= 'A' && _ch <= 'Z') ? _ch + 32 : _ch; }

/// @brief Converts an ASCII letter to upper case; leaves other characters unchanged.
/// @param _ch Input character.
/// @return Uppercased ASCII character or the original character.
constexpr char ToUpper(char _ch) { return (_ch >= 'a' && _ch <= 'z') ? _ch - 32 : _ch; }

/// @brief Checks whether a character is an ASCII lowercase letter.
/// @param _ch Input character.
/// @return true if `_ch` is in ['a'..'z'].
constexpr bool IsLower(char _ch) { return (_ch >= 'a' && _ch <= 'z'); }

/// @brief Checks whether a character is an ASCII uppercase letter.
/// @param _ch Input character.
/// @return true if `_ch` is in ['A'..'Z'].
constexpr bool IsUpper(char _ch) { return (_ch >= 'A' && _ch <= 'Z'); }

constexpr bool IsEmpty(const char* _text) { return !_text || !_text[0]; }

/// @}

/// @brief Formats a string using printf-like syntax.
///
/// @param _format_psz printf-like format string.
/// @return Formatted string; empty string if formatting fails.
///
/// @note
/// Implementation is compatible with C++17 and uses vsnprintf internally.
///
/// @code
/// std::string s = Format("%s-%d", "id", 42);  // "id-42"
/// @endcode
std::string Format(const char* _format_psz, ...);

std::string Escape(const std::string& _text, bool _escape);

/// @brief Trims characters from both ends of `_text`.
///
/// @param _text Input text.
/// @param _trim_chars Characters to trim (treated as a set).
/// @return View into `_text` with leading/trailing trim characters removed.
///         Returns empty view if the result is empty.
///
/// @warning Returned string_view references `_text`. Do not use it after `_text` is destroyed.
std::string_view Trim(std::string_view _text, std::string_view _trim_chars = kDefaultTrimChars);

/// @brief Splits `_text` by a single separator character.
///
/// @param _text Input text.
/// @param _sep Separator character.
/// @param _trim_entries If true, each token is trimmed with Trim().
/// @param _drop_empty If true, empty tokens are removed (after optional trimming).
/// @return Vector of tokens (string_view) referencing `_text`.
///
/// @note By default, empty tokens are preserved:
/// @code
/// Split(",a,,b,", ',') -> ["", "a", "", "b", ""]
/// @endcode
std::vector<std::string_view> Split(std::string_view _text,
                                       char             _sep,
                                       bool             _trim_entries = false,
                                       bool             _drop_empty   = false);

/// @brief Splits `_text` by any delimiter character contained in `_delim`.
///
/// @param _text Input text.
/// @param _delim Set of delimiter characters.
/// @param _trim_entries If true, each token is trimmed with Trim().
/// @param _drop_empty If true, empty tokens are removed (after optional trimming).
/// @return Vector of tokens (string_view) referencing `_text`.
std::vector<std::string_view> SplitAny(std::string_view _text,
                                          std::string_view _delim,
                                          bool             _trim_entries = false,
                                          bool             _drop_empty   = false);

/// @brief Splits `_text` by an exact delimiter substring `_delim`.
///
/// @param _text Input text.
/// @param _delim Delimiter substring.
/// @param _trim_entries If true, each token is trimmed with Trim().
/// @param _drop_empty If true, empty tokens are removed (after optional trimming).
/// @return Vector of tokens (string_view) referencing `_text`.
///
/// @note If `_delim` is empty, returns a single token equal to `_text`.
std::vector<std::string_view> SplitExact(std::string_view _text,
                                            std::string_view _delim,
                                            bool             _trim_entries = false,
                                            bool             _drop_empty   = false);

std::vector<std::string> SplitCopy(std::string_view _text, char _sep, bool _trim_entries = false);

std::vector<std::string> SplitAnyCopy(std::string_view _text, std::string_view _delim, bool _trim_entries = false);

std::vector<std::string> SplitExactCopy(std::string_view _text, std::string_view _delim, bool _keep_delim = false);

template <class TNumber>
std::vector<TNumber> SplitNumbers(const std::string& _text, const char _sep)
{
    std::vector<TNumber> tokens;
    if (!_text.empty()) {
        size_t next = 0;
        size_t pos  = 0;
        do {
            next              = _text.find(_sep, pos);
            const auto token  = std::string(Trim(std::string_view(_text).substr(pos, next - pos), " \f\v\r\n\t\"'"));
            const auto value  = token.empty() ? 0.0 : std::strtod(token.c_str(), nullptr);
            tokens.push_back(static_cast<TNumber>(value));
            pos = next + 1;
        } while (next != std::string::npos);
    }

    return tokens;
}

/// @brief Converts ASCII letters in `_text` to lower case.
/// @param _text Input text.
/// @return New string with ASCII letters lowercased.
std::string Lower(std::string_view _text);

/// @brief Converts ASCII letters in `_text` to upper case.
/// @param _text Input text.
/// @return New string with ASCII letters uppercased.
std::string Upper(std::string_view _text);

/// @brief Converts CamelCase/PascalCase/snake_case into snake_case.
///
/// @param _text Input text.
/// @return snake_case string.
///
/// @details
/// - Preserves existing '_' characters.
/// - Inserts '_' at word boundaries (e.g. `HTTPRequest` -> `http_request`).
/// - ASCII-only; non-ASCII code units are copied as-is.
///
/// @code
/// SnakeCase("SimpleTest")  -> "simple_test"
/// SnakeCase("HTTPRequest") -> "http_request"
/// @endcode
std::string SnakeCase(std::string_view _text);

/// @brief ASCII case-insensitive compare.
/// @param _str_left Left operand.
/// @param _str_right Right operand.
/// @return 0 if equal; <0 if left < right; >0 otherwise.
int32_t CmpI(std::string_view _str_left, std::string_view _str_right);

/// @brief Compare with optional case sensitivity.
///
/// @param _str_left Left operand.
/// @param _str_right Right operand.
/// @param _case_sensitive If true, performs case-sensitive comparison; otherwise ASCII-insensitive.
/// @return 0 if equal; <0 if left < right; >0 otherwise.
int32_t Cmp(std::string_view _str_left, std::string_view _str_right, bool _case_sensitive);

/// @brief Finds `_str_find` in `_text` starting from `_offset`.
///
/// @param _text Text to search within.
/// @param _str_find Substring to find.
/// @param _case_sensitive If false, uses ASCII case-insensitive search.
/// @param _offset Start offset.
/// @return Index of first occurrence or std::string_view::npos if not found.
size_t Find(std::string_view _text, std::string_view _str_find, bool _case_sensitive, size_t _offset);

inline bool IsSame(const std::string_view _left, const std::string_view _right, const bool _case_sensitive)
{
    return _case_sensitive ? _left == _right : CmpI(_left, _right) == 0;
}

/// @brief Checks whether `_text` starts with `_prefix` (ASCII case-insensitive).
///
/// @param _text Input text.
/// @param _prefix Prefix to check.
/// @param _wo_prefix_p Optional output: remainder of `_text` after prefix.
/// @return true if `_text` starts with `_prefix`, otherwise false.
///
/// @note If `_prefix` is empty, returns true.
bool IsPrefix(std::string_view _text, std::string_view _prefix, std::string* _wo_prefix_p);

/// @brief Convenience overload for IsPrefix().
/// @param _text Input text.
/// @param _prefix Prefix to check.
/// @return Pair {matched, remainder}.
std::pair<bool, std::string> IsPrefix(std::string_view _text, std::string_view _prefix);

/// @brief Checks whether `_text` ends with `_postfix` (ASCII case-insensitive).
///
/// @param _text Input text.
/// @param _postfix Postfix to check.
/// @param _wo_postfix_p Optional output: remainder of `_text` before postfix.
/// @return true if `_text` ends with `_postfix`, otherwise false.
///
/// @note If `_postfix` is empty, returns true.
bool IsPostfix(std::string_view _text, std::string_view _postfix, std::string* _wo_postfix_p);

/// @brief Convenience overload for IsPostfix().
/// @param _text Input text.
/// @param _postfix Postfix to check.
/// @return Pair {matched, remainder}.
std::pair<bool, std::string> IsPostfix(std::string_view _text, std::string_view _postfix);

std::string Replace(const std::string& _text, const std::string& _from, const std::string& _to, size_t* _replaced_p);

size_t Replace(std::string& _text, const std::string& _from, const std::string& _to);

/// @brief Joins parts into a single string.
///
/// @param _parts List of parts (string_view).
/// @param _drop_empty If true, empty parts are skipped.
/// @param _delimiter Inserted between parts (only between non-dropped parts).
/// @return Joined string.
///
/// @code
/// Join({"a","","b"}, true, ",")  -> "a,b"
/// Join({"a","","b"}, false, ",") -> "a,,b"
/// @endcode
std::string Join(const std::vector<std::string_view>& _parts, bool _drop_empty, std::string_view _delimiter = {});

std::string Join(const std::vector<std::string>& _parts, bool _drop_empty, std::string_view _delimiter = {});

inline std::string Join(const std::initializer_list<std::string_view> _parts,
                        const bool _drop_empty,
                        const std::string_view _delimiter = {})
{
    return Join(std::vector<std::string_view>(_parts), _drop_empty, _delimiter);
}

bool SaveFile(std::string_view        _text,
              std::string_view        _filename,
              std::ios_base::openmode _mode    = std::ios_base::out,
              std::string*            _error_p = nullptr);

std::string LoadFile(std::string_view        _filename,
                     std::ios_base::openmode _mode    = std::ios_base::in,
                     std::string*            _error_p = nullptr);

} // namespace xsdk::xbase::strings
