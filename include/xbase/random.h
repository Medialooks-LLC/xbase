#pragma once

#include "xbase/symbols.h"

#include <cstdint>
#include <string>

namespace xsdk::xbase::random {

XBASE_API double Value();

XBASE_API int32_t Int(double _max_value);

XBASE_API int32_t Int(int32_t _min_value, int32_t _max_value);

XBASE_API bool Bool(double _probability);

XBASE_API std::string UniqueString(
    uint16_t _len,
    const char* _chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

XBASE_API uint64_t UniqueUint64(bool _avoid_high_bit = false);

} // namespace xsdk::xbase::random
