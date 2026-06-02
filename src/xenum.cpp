#include "xbase/xenum.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iterator>
#include <vector>

namespace xsdk::xenum_detail {
namespace {

    struct EnumEntry {
        std::string_view name;
        int64_t          value = 0;
    };

    bool IsIdentFirstChar(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }

    bool IsIdentChar(char c) { return IsIdentFirstChar(c) || (c >= '0' && c <= '9'); }

    std::string_view Trim(std::string_view value)
    {
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0)
            value.remove_prefix(1);

        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0)
            value.remove_suffix(1);

        return value;
    }

    std::string_view StripOuterParens(std::string_view body)
    {
        body = Trim(body);
        assert(body.size() >= 2 && body.front() == '(' && body.back() == ')');
        body.remove_prefix(1);
        body.remove_suffix(1);
        return body;
    }

    std::string_view ReadEnumeratorName(std::string_view token)
    {
        token = Trim(token);
        if (token.empty() || !IsIdentFirstChar(token.front()))
            return {};

        size_t pos = 1;
        while (pos < token.size() && IsIdentChar(token[pos]))
            ++pos;

        return token.substr(0, pos);
    }

    template <class Callback>
    void ForEachEnumeratorToken(std::string_view body, Callback&& callback)
    {
        body = StripOuterParens(body);

        size_t token_start = 0;
        int    paren       = 0;
        int    bracket     = 0;
        int    brace       = 0;
        char   quote       = 0;
        bool   escape      = false;

        auto flush = [&](size_t token_end) { callback(Trim(body.substr(token_start, token_end - token_start))); };

        for (size_t i = 0; i < body.size(); ++i) {
            const char c = body[i];

            if (quote != 0) {
                if (escape) {
                    escape = false;
                }
                else if (c == '\\') {
                    escape = true;
                }
                else if (c == quote) {
                    quote = 0;
                }
                continue;
            }

            if (c == '\'' || c == '"') {
                quote = c;
                continue;
            }

            switch (c) {
                case '(':
                    ++paren;
                    break;
                case ')':
                    if (paren > 0)
                        --paren;
                    break;
                case '[':
                    ++bracket;
                    break;
                case ']':
                    if (bracket > 0)
                        --bracket;
                    break;
                case '{':
                    ++brace;
                    break;
                case '}':
                    if (brace > 0)
                        --brace;
                    break;
                case ',':
                    if (paren == 0 && bracket == 0 && brace == 0) {
                        flush(i);
                        token_start = i + 1;
                    }
                    break;
                default:
                    break;
            }
        }

        flush(body.size());
    }

} // namespace

struct EnumReflection::Impl {
    std::vector<EnumEntry> values;
    std::string_view       enum_name;
};

EnumReflection::EnumReflection(const int64_t* values, int32_t count, const char* enum_name, const char* enum_body)
    : impl_(std::make_unique<Impl>())
{
    impl_->enum_name = enum_name;
    impl_->values.reserve(static_cast<size_t>(count));

    int32_t value_index = 0;
    ForEachEnumeratorToken(enum_body, [&](std::string_view token) {
        if (token.empty())
            return;

        assert(value_index < count);
        if (value_index >= count)
            return;

        const auto name = ReadEnumeratorName(token);
        assert(!name.empty());
        if (name.empty())
            return;

        impl_->values.push_back({name, values[value_index]});
        ++value_index;
    });

    assert(value_index == count);
}

EnumReflection::~EnumReflection() = default;

int32_t EnumReflection::Count() const { return static_cast<int32_t>(impl_->values.size()); }

std::string_view EnumReflection::EnumName() const { return impl_->enum_name; }

std::optional<std::string_view> EnumReflection::NameByValue(int64_t value) const
{
    const auto it = std::find_if(impl_->values.begin(), impl_->values.end(), [&](const auto& item) {
        return item.value == value;
    });

    if (it == impl_->values.end())
        return std::nullopt;

    return it->name;
}

std::optional<int64_t> EnumReflection::ValueByName(std::string_view name) const
{
    const auto it = std::find_if(impl_->values.begin(), impl_->values.end(), [&](const auto& item) {
        return item.name == name;
    });

    if (it == impl_->values.end())
        return std::nullopt;

    return it->value;
}

} // namespace xsdk::xenum_detail
