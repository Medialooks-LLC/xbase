#pragma once

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
uint64_t NextUid();

// Invalud uid mark
static constexpr uint64_t kInvalidUid = 0;
// First possible uid - uids between kInvalidUid and kFirstUid could be used as service tags
static constexpr uint64_t kFirstUid = 1000;

// Return same unique value for same _group_uid
uint64_t MakeUid(uint64_t _group_uid);

// Return same unique value for same _uid_first and _uid_second
uint64_t MakeUid(const uint64_t _uid_first, const uint64_t _uid_second);

// Return same unique value for same string
uint64_t MakeUid(const std::string& _string_base);

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
 * @brief Obtain a compile-time name for a given C++ type.
 * @tparam T The C++ type for which to compute the UID. Note: Result is platform dependent
 * @return The name for the given type `T` as a compile-time constant.
 */
template <class T>
constexpr std::string_view TypeName() noexcept
{
#ifdef _MSC_VER
    // class std::basic_string_view<char,struct std::char_traits<char> > __cdecl xsdk::xbase::TypeName<class
    // TestClass<long>>(void) noexcept
    constexpr std::string_view name_str = __FUNCSIG__;
#else
    // std::string_view __cdecl xsdk::xbase::TypeName(void) [T = TestClass<int>]  LLVM
    // "std::string_view xsdk::xbase::TypeName() [T = xtest::TestClass2]" MacOS
    // "constexpr std::string_view xsdk::xbase::TypeName() [with T = TestClass<long int>; std::string_view =
    // std::basic_string_view<char>]" GCC Linux
    constexpr std::string_view name_str = __PRETTY_FUNCTION__;
#endif

    constexpr std::string_view prefix_msvc = "xsdk::xbase::TypeName<";

    constexpr auto pos_msvc = name_str.find(prefix_msvc);
    if constexpr (pos_msvc != std::string_view::npos) {

        // Remove function prefix
        constexpr auto wo_prefix = name_str.substr(pos_msvc + prefix_msvc.size());

        // Try remove MSVC enum/struct/class prefix
        constexpr std::array<std::string_view, 3> class_prefixes = {"enum ", "struct ", "class "};
        constexpr std::string_view                postfix_msvc   = ">(void) noexcept";

        for (const auto& name : class_prefixes) {
            if (wo_prefix.find(name) == 0) {
                auto wo_class = wo_prefix.substr(name.size());

                // Remove MSVC postfix
                return wo_class.substr(0, wo_class.rfind(postfix_msvc));
            }
        }

        // Remove MSVC postfix
        return wo_prefix.substr(0, wo_prefix.rfind(postfix_msvc));
    }
#ifdef _WIN32
    #pragma warning(push)
    #pragma warning(suppress : 4702) // warning C4702: unreachable code
#endif
    // Common prefix
    constexpr std::string_view prefix_others = "T = ";
    constexpr auto             pos           = name_str.find(prefix_others);
    if constexpr (pos == std::string_view::npos)
        return name_str; // Can't find prefix

    // Remove function prefix
    constexpr auto wo_prefix = name_str.substr(pos + prefix_others.size());

    // Remove common postfix
    constexpr std::string_view end_name_chars = "];";
    return wo_prefix.substr(0, wo_prefix.find_first_of(end_name_chars));
#ifdef _WIN32
    #pragma warning(pop)
#endif
}

/**
 * @brief Obtain a compile-time constant UID for a given C++ type.
 * This template function computes the UID for a given C++ type `T` using the FNV-1a 64-bit algorithm and returns it as
 * a compile-time constant. Note: Result is platform dependent
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
