#include "xbase/strings.h"

#include <algorithm> // std::equal
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>
#include <utility>

/// @file strings.cpp
/// @brief Implementation of xsdk::xbase::strings utilities.

namespace xsdk::xbase {

namespace detail {
    /// @brief Pushes a substring token into a vector, optionally trimming/dropping empties.
    /// @param tokens Vector to push into.
    /// @param _text Source text.
    /// @param _offset Start offset.
    /// @param _count Token length.
    /// @param _trim_entries Whether to trim the token.
    /// @param _drop_empty Whether to drop empty tokens.
    /// @return The produced token (or empty view if dropped).
    std::string_view PushSubStr(std::vector<std::string_view>& tokens,
                                const std::string_view         _text,
                                const size_t                   _offset,
                                const size_t                   _count,
                                const bool                     _trim_entries,
                                const bool                     _drop_empty)
    {
        auto token = _text.substr(_offset, _count);
        if (_trim_entries)
            token = strings::StrTrim(token);
        if (_drop_empty && token.empty())
            return {};

        tokens.push_back(token);
        return token;
    }
} // namespace detail

std::string strings::StrFormat(const char* _format_psz, ...)
{
    va_list vl;
    va_start(vl, _format_psz);
    const auto len = std::vsnprintf(nullptr, 0, _format_psz, vl);
    va_end(vl);
    if (len <= 0)
        return {};

    va_start(vl, _format_psz);
    std::string result(len, '\0');
    std::vsnprintf(result.data(), len + 1, _format_psz, vl);
    va_end(vl);

    return result;
}
std::string_view strings::StrTrim(const std::string_view _text, const std::string_view _trim_chars)
{
    size_t start = _text.find_first_not_of(_trim_chars);
    if (start == std::string::npos)
        return {};

    size_t end = _text.find_last_not_of(_trim_chars);
    assert(end != std::string::npos);

    return _text.substr(start, end - start + 1);
}

std::vector<std::string_view> strings::StrSplit(const std::string_view _text,
                                                const char             _sep,
                                                const bool             _trim_entries,
                                                const bool             _drop_empty)
{
    std::vector<std::string_view> tokens;
    if (!_text.empty()) {
        size_t next = 0, pos = 0;
        while (next != std::string::npos) {
            next = _text.find(_sep, pos);
            detail::PushSubStr(tokens, _text, pos, next - pos, _trim_entries, _drop_empty);
            pos = next + 1;
        }
    }

    return tokens;
}

std::vector<std::string_view> strings::StrSplitAny(const std::string_view _text,
                                                   const std::string_view _delim,
                                                   const bool             _trim_entries,
                                                   const bool             _drop_empty)
{
    if (_text.empty())
        return {};

    std::vector<std::string_view> tokens;
    size_t                        next = 0, pos = 0;
    while (next != std::string::npos) {
        next = _text.find_first_of(_delim, pos);
        detail::PushSubStr(tokens, _text, pos, next - pos, _trim_entries, _drop_empty);
        pos = next + 1;
    }

    return tokens;
}

std::vector<std::string_view> strings::StrSplitExact(const std::string_view _text,
                                                     const std::string_view _delim,
                                                     const bool             _trim_entries,
                                                     const bool             _drop_empty)
{
    if (_delim.empty())
        return {_text};

    std::vector<std::string_view> tokens;
    size_t                        token_start = 0;
    size_t                        token_end   = 0;
    while (token_start < _text.length()) {
        token_end = _text.find(_delim, token_start);
        detail::PushSubStr(tokens, _text, token_start, token_end - token_start, _trim_entries, _drop_empty);
        token_start = token_end + (token_end == std::string::npos ? 0 : _delim.length());
    }

    return tokens;
}

std::string strings::StrLower(const std::string_view _text)
{
    std::string res(_text);
    std::transform(res.begin(), res.end(), res.begin(), strings::ToLower);
    return res;
}

std::string strings::StrUpper(const std::string_view _text)
{
    std::string res(_text);
    std::transform(res.begin(), res.end(), res.begin(), strings::ToUpper);
    return res;
}

std::string strings::StrSnakeCase(const std::string_view _text)
{
    // For out-of string return upper case for correct handle e.g. 'HTTP' conversion
    const auto char_at_pos = [](const std::string_view _text, const size_t _idx) {
        return _idx < _text.length() ? _text[_idx] : 'A';
    };

    std::ostringstream snake_case_out;
    for (size_t z = 0; z < _text.length(); ++z) {
        if (strings::IsUpper(_text[z])) {
            if (z > 0 && _text[z - 1] != '_' && char_at_pos(_text, z + 1) != '_' &&
                (!strings::IsUpper(_text[z - 1]) || !strings::IsUpper(char_at_pos(_text, z + 1)))) {
                snake_case_out << "_";
            }

            snake_case_out << strings::ToLower(_text[z]);
        }
        else {
            snake_case_out << _text[z];
        }
    }

    return std::move(snake_case_out).str();
}

int32_t strings::StrCmpI(const std::string_view _str_left, const std::string_view _str_right)
{
    if (_str_left.length() != _str_right.length())
        return (int)_str_left.length() - (int)_str_right.length();

    for (size_t z = 0; z < _str_left.length(); ++z) {
        auto cmp = strings::ToLower(_str_left[z]) - strings::ToLower(_str_right[z]);
        if (cmp != 0)
            return cmp;
    }

    return 0;
}

int32_t strings::StrCmp(const std::string_view _str_left, const std::string_view _str_right, const bool _case_sensitive)
{
    if (_case_sensitive)
        return _str_left.compare(_str_right);

    return strings::StrCmpI(_str_left, _str_right);
}

size_t strings::StrFind(const std::string_view _text,
                        const std::string_view _str_find,
                        const bool             _case_sensitive,
                        const size_t           _offset)
{
    if (_case_sensitive)
        return _text.find(_str_find, _offset);

    auto text     = strings::StrLower(_text);
    auto str_find = strings::StrLower(_str_find);
    return text.find(str_find, _offset);
}

bool strings::StrIsPrefix(const std::string_view _text, const std::string_view _prefix, std::string* const _wo_prefix_p)
{
    if (_prefix.empty() || _text.empty())
        return _prefix.empty() ? true : false;

    auto text_prefix = _text.substr(0, _prefix.length());
    if (strings::StrCmpI(text_prefix, _prefix))
        return false;

    if (_wo_prefix_p)
        *_wo_prefix_p = _text.substr(_prefix.length());

    return true;
}

std::pair<bool, std::string> strings::StrIsPrefix(const std::string_view _text, const std::string_view _prefix)
{
    std::string rest_part;
    if (!strings::StrIsPrefix(_text, _prefix, &rest_part))
        return {false, {}};

    return {true, std::move(rest_part)};
}

bool strings::StrIsPostfix(const std::string_view _text,
                           const std::string_view _postfix,
                           std::string* const     _wo_postfix_p)
{
    if (_postfix.empty() || _text.empty())
        return _postfix.empty() ? true : false;

    auto text_len    = _text.length();
    auto postfix_len = _postfix.length();
    if (text_len < postfix_len)
        return false;

    if (strings::StrCmpI(_text.substr(text_len - postfix_len), _postfix))
        return false;

    if (_wo_postfix_p)
        *_wo_postfix_p = _text.substr(0, text_len - postfix_len);

    return true;
}

std::pair<bool, std::string> strings::StrIsPostfix(const std::string_view _text, const std::string_view _postfix)
{
    std::string rest_part;
    if (!strings::StrIsPostfix(_text, _postfix, &rest_part))
        return {false, {}};

    return {true, std::move(rest_part)};
}

std::string strings::StrJoin(const std::vector<std::string_view>& _parts,
                             const bool                           _drop_empty,
                             const std::string_view               _delimiter)
{
    std::ostringstream out;
    bool               is_first = true;
    for (const auto& part : _parts) {
        if (_drop_empty && part.empty())
            continue;

        if (!_delimiter.empty() && !std::exchange(is_first, false))
            out << _delimiter;

        out << part;
    }

    return std::move(out).str();
}

} // namespace xsdk::xbase
