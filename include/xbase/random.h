#pragma once

#include <cstdint>
#include <string>

namespace xsdk::xbase::random {

double Value();

int32_t Int(double _max_value);

int32_t Int(int32_t _min_value, int32_t _max_value);

bool Bool(double _probability);

std::string UniqueString(uint16_t _len,
                         const char* _chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

uint64_t UniqueUint64(bool _avoid_high_bit = false);

} // namespace xsdk::xbase::random
