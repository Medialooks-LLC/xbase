#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace xsdk::xbase::base64 {

bool IsBase64(uint8_t _ch);

std::string Encode(const void* _data, size_t _size);

std::string Encode(const std::vector<uint8_t>& _data);

std::string EncodeString(std::string_view _data);

std::vector<uint8_t> Decode(std::string_view _encoded, size_t _len = std::string_view::npos);

std::string DecodeString(std::string_view _encoded, size_t _len = std::string_view::npos);

} // namespace xsdk::xbase::base64
