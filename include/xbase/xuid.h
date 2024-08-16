#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace xsdk::xbase {

using Uid = std::uint64_t;

/**
 * @brief Generate the next unique number.
 *
 * This function generates and returns the next unique number.
 */
uint64_t NextUid();

// From here:
// https://stackoverflow.com/questions/48896142/is-it-possible-to-get-hash-values-as-compile-time-constants
// https://stackoverflow.com/questions/56292104/hashing-types-at-compile-time-in-c17-c2a
/**
 * @brief Hash a string using FNV-1a 64-bit algorithm.
 * This function computes the hash value for a given string using the FNV-1a 64-bit algorithm.
 * @param _to_hash The string to be hashed.
 * @return The computed hash value as a compile-time constant.
 */
constexpr uint64_t HashString(std::string_view _to_hash)
{
    // FNV-1a 64 bit algorithm
    uint64_t result = 0xcbf29ce484222325; // FNV offset basis

    for (const char c : _to_hash) {
        result ^= c;
        result *= 1099511628211; // FNV prime
    }

    return result;
}

/**
 * @brief Obtain a compile-time constant UID for a given C++ type.
 * This template function computes the UID for a given C++ type `T` using the FNV-1a 64-bit algorithm and returns it as
 * a compile-time constant.
 * @tparam T The C++ type for which to compute the UID.
 * @return The UID for the given type `T` as a compile-time constant.
 */
template <class T>
constexpr xbase::Uid TypeUid() noexcept
{
#ifdef _MSC_VER
    const std::string_view uid_str = __FUNCSIG__;
#else
    const std::string_view uid_str = __PRETTY_FUNCTION__;
#endif
    return HashString(uid_str);
}

} // namespace xsdk::xbase
