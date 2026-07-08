#pragma once

#include "xbase/symbols.h"
#include "internal/xuid_type_name.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace xsdk::xbase {

using Uid = std::uint64_t;

/**
 * @brief Generate the next unique number.
 *
 * This function generates and returns the next unique number.
 */
XBASE_API uint64_t NextUid();

// Invalud uid mark
static constexpr uint64_t kInvalidUid = 0;
// First possible uid - uids between kInvalidUid and kFirstUid could be used as service tags
static constexpr uint64_t kFirstUid = 1000;

// Return same unique value for same _group_uid
XBASE_API uint64_t MakeUid(uint64_t _group_uid);

// Return same unique value for same _uid_first and _uid_second
XBASE_API uint64_t MakeUid(const uint64_t _uid_first, const uint64_t _uid_second);

// Return same unique value for same string
XBASE_API uint64_t MakeUid(const std::string& _string_base);

// From here:
// https://stackoverflow.com/questions/48896142/is-it-possible-to-get-hash-values-as-compile-time-constants
// https://stackoverflow.com/questions/56292104/hashing-types-at-compile-time-in-c17-c2a
/**
 * @brief Hash a string using FNV-1a 64-bit algorithm
 *
 * This function computes the hash value for a given string using the FNV-1a 64-bit algorithm and optionally
 * continues hashing from a previously computed value.
 *
 * @param _to_hash The string to be hashed.
 * @param _continue_from Optional previous hash value used to continue hashing.
 * @return The computed hash value as a compile-time constant.
 */
constexpr uint64_t HashString(const std::string_view        _to_hash,
                              const std::optional<uint64_t> _continue_from = {}) noexcept
{
    // FNV-1a 64 bit algorithm
    // FNV offset basis: 0xcbf29ce484222325 (14695981039346656037)
    uint64_t result = _continue_from.value_or(0xcbf29ce484222325);

    for (const char c : _to_hash) {
        result ^= static_cast<unsigned char>(c);
        result *= 1099511628211ULL; // FNV prime
    }

    return result;
}

/**
 * @brief Hash raw byte data using the FNV-1a 64-bit algorithm.
 *
 * This function hashes `_bytes` bytes starting at `_data_p` and optionally
 * continues hashing from a previously computed value.
 *
 * @param _data_p Pointer to the raw data buffer to hash.
 * @param _bytes Number of bytes to hash from `_data_p`.
 * @param _continue_from Optional previous hash value used to continue hashing.
 * @return The computed 64-bit hash value, or `xbase::kInvalidUid` if `_data_p` is `nullptr`.
 *
 * @note This function treats the input as raw bytes. The caller must ensure that
 *       `_data_p` points to at least `_bytes` readable bytes.
 */
inline uint64_t HashData(const void* _data_p, const size_t _bytes, const std::optional<uint64_t> _continue_from = {})
{
    if (!_data_p)
        return xbase::kInvalidUid;

    return HashString({reinterpret_cast<const char*>(_data_p), _bytes}, _continue_from);
}

/**
 * @brief Hash raw object bytes using the FNV-1a 64-bit algorithm.
 *
 * This template hashes the in-memory byte representation of `_data` and optionally
 * continues hashing from a previously computed value.
 *
 * @tparam TData Object type to hash. Must be `std::is_trivially_copyable_v<TData>`.
 * @param _data Object whose raw bytes will be hashed.
 * @param _continue_from Optional previous hash value used to continue hashing.
 * @return The computed 64-bit hash value.
 *
 * @note This function hashes the binary representation of the object, not its logical value.
 *       The result may depend on object layout, padding, compiler, platform, and endianness.
 */
template <class TData>
uint64_t HashDataT(const TData& _data, const std::optional<uint64_t> _continue_from = {}) noexcept
{
    static_assert(std::is_trivially_copyable_v<TData>, "HashDataT requires TData to be trivially copyable.");

    return HashString({reinterpret_cast<const char*>(&_data), sizeof(_data)}, _continue_from);
}

/**
 * @brief Obtain a canonical compile-time name for a given C++ type.
 *
 * The returned name is normalized from the compiler-specific spelling and is intended
 * to be stable across supported compilers for named user-defined types used as
 * interface keys, including top-level `const`.
 *
 * This is intended for interface-like named types queried across binary boundaries;
 * it is not a general canonicalizer for every possible C++ type spelling.
 *
 * @tparam T The C++ type for which to compute the canonical name.
 * @return The canonical name for the given type `T` as a compile-time constant.
 */
template <class T>
constexpr std::string_view TypeName() noexcept
{
    return impl::CanonicalTypeNameStorage<T>::value;
}

/**
 * @brief Obtain a compile-time constant UID for a given C++ type.
 * This template function computes the UID for a given C++ type `T` by hashing the canonical
 * result of `TypeName<T>()` using the FNV-1a 64-bit algorithm.
 *
 * Rebuilding all producers and consumers is required whenever canonical type naming rules
 * change, because the resulting `TypeUid<T>()` values change as well.
 *
 * @tparam T The C++ type for which to compute the UID.
 * @return The UID for the given type `T` as a compile-time constant.
 */
template <class T>
constexpr xbase::Uid TypeUid() noexcept
{
    constexpr uint64_t hash = HashString(TypeName<T>());
    return hash;
}

} // namespace xsdk::xbase
