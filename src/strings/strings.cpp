#include "xbase/strings.h"

#include <algorithm> // std::equal
#include <cassert>
#include <cstring>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <fstream>
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
            token = strings::Trim(token);
        if (_drop_empty && token.empty())
            return {};

        tokens.push_back(token);
        return token;
    }
} // namespace detail

std::string strings::Format(const char* _format_psz, ...)
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

std::string strings::Escape(const std::string& _text, const bool _escape)
{
    const char  escape_chars[] = {'\b', '\f', '\n', '\r', '\t', '\"', '\\'};
    const char* escaped_str[]  = {"\\b", "\\f", "\\n", "\\r", "\\t", "\\\"", "\\\\"};

    std::vector<char> result(_text.length() * 2 + 1);
    char*             dst = result.data();
    const char*       src = _text.c_str();

    if (_escape) {
        while (*src) {
            int32_t escape_idx = -1;
            for (int32_t i = 0; i < static_cast<int32_t>(sizeof(escape_chars) / sizeof(escape_chars[0])); ++i) {
                if (*src == escape_chars[i]) {
                    escape_idx = i;
                    break;
                }
            }

            if (escape_idx >= 0) {
                *(dst++) = escaped_str[escape_idx][0];
                *(dst++) = escaped_str[escape_idx][1];
                ++src;
            }
            else {
                *(dst++) = *(src++);
            }
        }
    }
    else {
        while (*src) {
            int32_t escape_idx = -1;
            for (int32_t i = 0; i < static_cast<int32_t>(sizeof(escape_chars) / sizeof(escape_chars[0])); ++i) {
                if (src[0] == escaped_str[i][0] && src[1] == escaped_str[i][1]) {
                    escape_idx = i;
                    break;
                }
            }

            if (escape_idx >= 0) {
                *(dst++) = escape_chars[escape_idx];
                src += 2;
            }
            else {
                *(dst++) = *(src++);
            }
        }
    }

    return std::string(result.data());
}
std::string_view strings::Trim(const std::string_view _text, const std::string_view _trim_chars)
{
    size_t start = _text.find_first_not_of(_trim_chars);
    if (start == std::string::npos)
        return {};

    size_t end = _text.find_last_not_of(_trim_chars);
    assert(end != std::string::npos);

    return _text.substr(start, end - start + 1);
}

std::vector<std::string_view> strings::Split(const std::string_view _text,
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

std::vector<std::string_view> strings::SplitAny(const std::string_view _text,
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

std::vector<std::string_view> strings::SplitExact(const std::string_view _text,
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

std::vector<std::string> strings::SplitCopy(const std::string_view _text, const char _sep, const bool _trim_entries)
{
    std::vector<std::string> tokens;
    for (const auto part : strings::Split(_text, _sep, _trim_entries))
        tokens.emplace_back(part);
    return tokens;
}

std::vector<std::string> strings::SplitAnyCopy(const std::string_view _text,
                                               const std::string_view _delim,
                                               const bool             _trim_entries)
{
    std::vector<std::string> tokens;
    for (const auto part : strings::SplitAny(_text, _delim, _trim_entries))
        tokens.emplace_back(part);
    return tokens;
}

std::vector<std::string> strings::SplitExactCopy(const std::string_view _text,
                                                 const std::string_view _delim,
                                                 const bool             _keep_delim)
{
    if (_delim.empty())
        return _text.empty() ? std::vector<std::string>() : std::vector<std::string> {std::string(_text)};

    std::vector<std::string> tokens;
    size_t                   prev = 0;
    size_t                   pos  = 0;
    do {
        pos = _text.find(_delim, prev);
        if (pos == std::string::npos)
            pos = _text.length();

        std::string token(_text.substr(prev, pos - prev));
        if (!token.empty()) {
            if (_keep_delim && !tokens.empty())
                token = std::string(_delim) + token;
            tokens.push_back(std::move(token));
        }

        prev = pos + _delim.length();
    } while (pos < _text.length() && prev < _text.length());

    return tokens;
}

std::string strings::Lower(const std::string_view _text)
{
    std::string res(_text);
    std::transform(res.begin(), res.end(), res.begin(), strings::ToLower);
    return res;
}

std::string strings::Upper(const std::string_view _text)
{
    std::string res(_text);
    std::transform(res.begin(), res.end(), res.begin(), strings::ToUpper);
    return res;
}

std::string strings::SnakeCase(const std::string_view _text)
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

int32_t strings::CmpI(const std::string_view _str_left, const std::string_view _str_right)
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

int32_t strings::Cmp(const std::string_view _str_left, const std::string_view _str_right, const bool _case_sensitive)
{
    if (_case_sensitive)
        return _str_left.compare(_str_right);

    return strings::CmpI(_str_left, _str_right);
}

size_t strings::Find(const std::string_view _text,
                        const std::string_view _str_find,
                        const bool             _case_sensitive,
                        const size_t           _offset)
{
    if (_case_sensitive)
        return _text.find(_str_find, _offset);

    auto text     = strings::Lower(_text);
    auto str_find = strings::Lower(_str_find);
    return text.find(str_find, _offset);
}

bool strings::IsPrefix(const std::string_view _text, const std::string_view _prefix, std::string* const _wo_prefix_p)
{
    if (_prefix.empty() || _text.empty())
        return _prefix.empty() ? true : false;

    auto text_prefix = _text.substr(0, _prefix.length());
    if (strings::CmpI(text_prefix, _prefix))
        return false;

    if (_wo_prefix_p)
        *_wo_prefix_p = _text.substr(_prefix.length());

    return true;
}

std::pair<bool, std::string> strings::IsPrefix(const std::string_view _text, const std::string_view _prefix)
{
    std::string rest_part;
    if (!strings::IsPrefix(_text, _prefix, &rest_part))
        return {false, {}};

    return {true, std::move(rest_part)};
}

bool strings::IsPostfix(const std::string_view _text,
                           const std::string_view _postfix,
                           std::string* const     _wo_postfix_p)
{
    if (_postfix.empty() || _text.empty())
        return _postfix.empty() ? true : false;

    auto text_len    = _text.length();
    auto postfix_len = _postfix.length();
    if (text_len < postfix_len)
        return false;

    if (strings::CmpI(_text.substr(text_len - postfix_len), _postfix))
        return false;

    if (_wo_postfix_p)
        *_wo_postfix_p = _text.substr(0, text_len - postfix_len);

    return true;
}

std::pair<bool, std::string> strings::IsPostfix(const std::string_view _text, const std::string_view _postfix)
{
    std::string rest_part;
    if (!strings::IsPostfix(_text, _postfix, &rest_part))
        return {false, {}};

    return {true, std::move(rest_part)};
}

std::string strings::Replace(const std::string& _text,
                             const std::string& _from,
                             const std::string& _to,
                             size_t* const      _replaced_p)
{
    if (_text.empty() || _from.empty() || _from == _to) {
        if (_replaced_p)
            *_replaced_p = 0;
        return _text;
    }

    std::string result;
    size_t      pos   = 0;
    size_t      count = 0;
    while (true) {
        const size_t next   = _text.find(_from, pos);
        const size_t length = next == std::string::npos ? _text.length() - pos : next - pos;
        result += _text.substr(pos, length);
        if (next == std::string::npos)
            break;

        result += _to;
        pos = next + _from.length();
        ++count;
    }

    if (_replaced_p)
        *_replaced_p = count;

    return result;
}

size_t strings::Replace(std::string& _text, const std::string& _from, const std::string& _to)
{
    size_t count = 0;
    _text        = strings::Replace(_text, _from, _to, &count);
    return count;
}

std::string strings::Join(const std::vector<std::string_view>& _parts,
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

std::string strings::Join(const std::vector<std::string>& _parts,
                          const bool                      _drop_empty,
                          const std::string_view          _delimiter)
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

bool strings::SaveFile(const std::string_view  _text,
                       const std::string_view  _filename,
                       const std::ios_base::openmode _mode,
                       std::string* const      _error_p)
{
    const std::string filename(_filename);
    try {
        std::ofstream out(filename, _mode);
        if (!out.is_open()) {
            if (_error_p)
                *_error_p = "open file:'" + filename + "' failed.";
            return false;
        }

        out.write(_text.data(), static_cast<std::streamsize>(_text.size()));
        if (out.fail()) {
            if (_error_p)
                *_error_p = "write to:'" + filename + "' failed.";
            return false;
        }
    }
    catch (const std::exception& ex) {
        if (_error_p)
            *_error_p = ex.what();
        return false;
    }

    return true;
}

std::string strings::LoadFile(const std::string_view  _filename,
                              const std::ios_base::openmode _mode,
                              std::string* const      _error_p)
{
    const std::string filename(_filename);
    try {
        std::ifstream input(filename, _mode);
        if (!input.is_open()) {
            if (_error_p)
                *_error_p = "open file:'" + filename + "' failed.";
            return {};
        }

        input.seekg(0, std::ios::end);
        const auto len = input.tellg();
        if (len == std::ifstream::pos_type(-1)) {
            if (_error_p)
                *_error_p = "tellg() for:'" + filename + "' failed.";
            return {};
        }

        std::string result;
        if (len > 0) {
            result.resize(static_cast<size_t>(len));
            input.seekg(0, std::ios::beg);
            const auto actual_read = input.rdbuf()->sgetn(result.data(), len);
            result.resize(static_cast<size_t>(actual_read));
        }
        return result;
    }
    catch (const std::exception& ex) {
        if (_error_p)
            *_error_p = ex.what();
        return {};
    }
}

} // namespace xsdk::xbase
