#pragma once

#include "xbase/symbols.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace xsdk::xbase::base64 {

XBASE_API bool IsBase64(uint8_t _ch);

XBASE_API std::string Encode(const void* _data, size_t _size);

XBASE_API std::string Encode(const std::vector<uint8_t>& _data);

XBASE_API std::string EncodeString(std::string_view _data);

XBASE_API std::vector<uint8_t> Decode(std::string_view _encoded, size_t _len = std::string_view::npos);

XBASE_API std::string DecodeString(std::string_view _encoded, size_t _len = std::string_view::npos);

} // namespace xsdk::xbase::base64
