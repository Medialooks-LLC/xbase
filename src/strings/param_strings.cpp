#include "xbase/param_strings.h"

#include <memory>
#include <utility>

namespace xsdk::xbase {
namespace {

    bool IsWhiteSpace(const char _ch)
    {
        switch (_ch) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
            case '\f':
            case '\v':
                return true;
            default:
                return false;
        }
    }

    bool IsQuote(const char _ch) { return _ch == '\'' || _ch == '"'; }

    void SkipWhiteSpace(const std::string_view _text, size_t* const _pos)
    {
        while (*_pos < _text.size() && IsWhiteSpace(_text[*_pos]))
            ++(*_pos);
    }

    ParamTokenView MakeToken(const std::string_view _text, const size_t _begin, const size_t _end)
    {
        return ParamTokenView {_text.substr(_begin, _end - _begin), _begin, _end};
    }

    void AddError(ParamParseResult* const   _result,
                  const ParamParseErrorCode _code,
                  const size_t              _position,
                  const ParamTokenView&     _key   = {},
                  const ParamTokenView&     _token = {})
    {
        _result->errors.push_back(ParamParseError {_code, _position, _key, _token});
    }

    bool SkipQuotedToken(const std::string_view _text, size_t* const _pos, const char _quote)
    {
        while (*_pos < _text.size()) {
            const char ch = _text[*_pos];
            if (ch == '\\') {
                ++(*_pos);
                if (*_pos < _text.size())
                    ++(*_pos);
                continue;
            }

            if (ch == _quote) {
                ++(*_pos);
                return true;
            }

            ++(*_pos);
        }

        return false;
    }

    void SkipBrokenTokenRemainder(const std::string_view _text, size_t* const _pos)
    {
        if (*_pos >= _text.size())
            return;

        if (IsQuote(_text[*_pos])) {
            const char quote = _text[*_pos];
            ++(*_pos);
            SkipQuotedToken(_text, _pos, quote);
            return;
        }

        while (*_pos < _text.size() && !IsWhiteSpace(_text[*_pos]))
            ++(*_pos);
    }

    void SkipBrokenAssignmentValue(const std::string_view _text, size_t* const _pos)
    {
        SkipWhiteSpace(_text, _pos);
        SkipBrokenTokenRemainder(_text, _pos);
    }

    bool ReadKey(const std::string_view  _text,
                 size_t* const           _pos,
                 ParamTokenView* const   _key,
                 ParamParseResult* const _result)
    {
        if (*_pos >= _text.size())
            return false;

        if (!IsQuote(_text[*_pos])) {
            const size_t key_begin = *_pos;
            while (*_pos < _text.size() && !IsWhiteSpace(_text[*_pos]) && _text[*_pos] != '=')
                ++(*_pos);

            *_key = MakeToken(_text, key_begin, *_pos);
            if (_key->Empty()) {
                AddError(_result, ParamParseErrorCode::kEmptyKey, key_begin);
                return false;
            }

            return true;
        }

        const char   quote       = _text[*_pos];
        const size_t quote_begin = *_pos;
        const size_t key_begin   = *_pos + 1;

        ++(*_pos);
        const bool closed = SkipQuotedToken(_text, _pos, quote);
        if (!closed) {
            AddError(_result,
                     ParamParseErrorCode::kUnterminatedQuotedKey,
                     quote_begin,
                     {},
                     MakeToken(_text, quote_begin, _text.size()));
            return false;
        }

        const size_t key_end = *_pos - 1;
        *_key                = MakeToken(_text, key_begin, key_end);

        if (_key->Empty()) {
            AddError(_result, ParamParseErrorCode::kEmptyKey, quote_begin, {}, *_key);

            if (*_pos < _text.size() && !IsWhiteSpace(_text[*_pos]) && _text[*_pos] != '=') {
                SkipBrokenTokenRemainder(_text, _pos);
                return false;
            }

            size_t next_pos = *_pos;
            SkipWhiteSpace(_text, &next_pos);
            if (next_pos < _text.size() && _text[next_pos] == '=') {
                ++next_pos;
                SkipBrokenAssignmentValue(_text, &next_pos);
            }

            *_pos = next_pos;
            return false;
        }

        if (*_pos < _text.size() && !IsWhiteSpace(_text[*_pos]) && _text[*_pos] != '=') {
            AddError(_result,
                     ParamParseErrorCode::kUnexpectedCharacter,
                     *_pos,
                     *_key,
                     MakeToken(_text, *_pos, *_pos + 1));
            SkipBrokenTokenRemainder(_text, _pos);
            return false;
        }

        return true;
    }

    bool LooksLikeAssignmentAt(const std::string_view _text, const size_t _pos)
    {
        size_t pos = _pos;

        if (pos >= _text.size() || _text[pos] == '=')
            return false;

        if (IsQuote(_text[pos])) {
            const char quote = _text[pos];
            ++pos;

            const bool closed = SkipQuotedToken(_text, &pos, quote);
            if (!closed)
                return false;

            if (pos < _text.size() && !IsWhiteSpace(_text[pos]) && _text[pos] != '=')
                return false;
        }
        else {
            const size_t key_begin = pos;
            while (pos < _text.size() && !IsWhiteSpace(_text[pos]) && _text[pos] != '=')
                ++pos;

            if (key_begin == pos)
                return false;
        }

        SkipWhiteSpace(_text, &pos);
        return pos < _text.size() && _text[pos] == '=';
    }

    ParamParseResult PrepareResult(const std::string_view _text, const ParamParseOptions _options)
    {
        if (!_options.copy_input)
            return ParamParseResult {};

        if (_text.empty())
            return ParamParseResult(std::make_shared<const std::string>());

        return ParamParseResult(std::make_shared<const std::string>(_text.data(), _text.size()));
    }

    ParamParseResult ParseParamStringImpl(const std::string_view _text, const ParamParseOptions _options)
    {
        auto result = PrepareResult(_text, _options);

        const std::string_view text = result.SourceView(_text);

        size_t pos = 0;
        while (true) {
            SkipWhiteSpace(text, &pos);
            if (pos >= text.size())
                break;

            if (text[pos] == '=') {
                AddError(&result, ParamParseErrorCode::kEmptyKey, pos);
                ++pos;
                if (pos < text.size() && !IsWhiteSpace(text[pos]))
                    SkipBrokenTokenRemainder(text, &pos);
                continue;
            }

            ParamTokenView key_token;
            if (!ReadKey(text, &pos, &key_token, &result))
                continue;

            SkipWhiteSpace(text, &pos);

            if (pos >= text.size() || text[pos] != '=') {
                result.flags.push_back(ParamStringFlag {key_token});
                continue;
            }

            ++pos;

            const size_t empty_value_pos        = pos;
            const bool   has_space_after_equals = pos < text.size() && IsWhiteSpace(text[pos]);
            size_t       value_pos              = pos;

            SkipWhiteSpace(text, &value_pos);

            if (value_pos >= text.size()) {
                result.items.push_back(ParamStringItem {key_token, MakeToken(text, empty_value_pos, empty_value_pos)});
                break;
            }

            if (has_space_after_equals && LooksLikeAssignmentAt(text, value_pos)) {
                result.items.push_back(ParamStringItem {key_token, MakeToken(text, empty_value_pos, empty_value_pos)});
                pos = value_pos;
                continue;
            }

            pos = value_pos;

            if (IsQuote(text[pos])) {
                const char   quote       = text[pos];
                const size_t quote_begin = pos;
                const size_t value_begin = pos + 1;

                ++pos;
                const bool closed = SkipQuotedToken(text, &pos, quote);
                if (!closed) {
                    AddError(&result,
                             ParamParseErrorCode::kUnterminatedQuotedValue,
                             quote_begin,
                             key_token,
                             MakeToken(text, quote_begin, text.size()));
                    break;
                }

                const size_t value_end = pos - 1;
                result.items.push_back(ParamStringItem {key_token, MakeToken(text, value_begin, value_end)});
                continue;
            }

            const size_t value_begin = pos;
            while (pos < text.size() && !IsWhiteSpace(text[pos]))
                ++pos;

            const size_t value_end = pos;
            result.items.push_back(ParamStringItem {key_token, MakeToken(text, value_begin, value_end)});
        }

        return result;
    }

} // namespace

ParamParseResult ParseParamString(const std::string_view _text, const ParamParseOptions _options)
{
    return ParseParamStringImpl(_text, _options);
}

ParamParseResult ParseParamString(const char* _text, const ParamParseOptions _options)
{
    if (!_text) {
        ParamParseResult result;
        AddError(&result, ParamParseErrorCode::kNullInput, 0);
        return result;
    }

    return ParseParamStringImpl(std::string_view(_text), _options);
}

} // namespace xsdk::xbase
