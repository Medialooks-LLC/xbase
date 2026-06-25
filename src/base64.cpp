#include "xbase/base64.h"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace xsdk::xbase::base64 {

namespace {
constexpr std::string_view kBase64Chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";
} // namespace

bool IsBase64(const uint8_t _ch) { return std::isalnum(_ch) || _ch == '+' || _ch == '/'; }

std::string Encode(const void* const _data, size_t _size)
{
    if (!_size || !_data)
        return "";

    const auto* data = static_cast<const uint8_t*>(_data);

    std::string result;
    int32_t     i                  = 0;
    int32_t     j                  = 0;
    uint8_t     char_array_3[3]    = {};
    uint8_t     char_array_4[4]    = {};

    while (_size--) {
        char_array_3[i++] = *(data++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; ++i)
                result += kBase64Chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; ++j)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; j < i + 1; ++j)
            result += kBase64Chars[char_array_4[j]];
        while (i++ < 3)
            result += '=';
    }

    return result;
}

std::string Encode(const std::vector<uint8_t>& _data) { return Encode(_data.data(), _data.size()); }

std::string EncodeString(const std::string_view _data) { return Encode(_data.data(), _data.size()); }

std::vector<uint8_t> Decode(const std::string_view _encoded, const size_t _len)
{
    std::vector<uint8_t> result;
    if (_encoded.empty())
        return result;

    size_t  in_len          = std::min(_len, _encoded.size());
    int32_t i               = 0;
    int32_t j               = 0;
    int32_t in              = 0;
    uint8_t char_array_4[4] = {};
    uint8_t char_array_3[3] = {};

    while (in_len-- && _encoded[in] != '=' && IsBase64(_encoded[in])) {
        char_array_4[i++] = _encoded[in++];
        if (i == 4) {
            for (i = 0; i < 4; ++i)
                char_array_4[i] = static_cast<uint8_t>(kBase64Chars.find(char_array_4[i]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; ++i)
                result.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; ++j)
            char_array_4[j] = 0;
        for (j = 0; j < 4; ++j)
            char_array_4[j] = static_cast<uint8_t>(kBase64Chars.find(char_array_4[j]));

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; j < i - 1; ++j)
            result.push_back(char_array_3[j]);
    }

    return result;
}

std::string DecodeString(const std::string_view _encoded, const size_t _len)
{
    auto decoded = Decode(_encoded, _len);
    return decoded.empty() ? std::string() : std::string(reinterpret_cast<char*>(decoded.data()), decoded.size());
}

} // namespace xsdk::xbase::base64
