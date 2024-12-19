#pragma once

#include <array>
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

// Invalud uid mark
static constexpr uint64_t kInvalidUid = 0;
// First possible uid - uids between kInvalidUid and kFirstUid could be used as service tags
static constexpr uint64_t kFirstUid   = 1000;

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
 * @brief Hash a string using FNV-1a 64-bit algorithm.
 * This function computes the hash value for a given string using the FNV-1a 64-bit algorithm.
 * @param _to_hash The string to be hashed.
 * @return The computed hash value as a compile-time constant.
 */
constexpr uint64_t HashString(const std::string_view _to_hash) noexcept
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
    if (pos_msvc != std::string_view::npos) {

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

    // Common prefix
    constexpr std::string_view prefix_others = "T = ";
    constexpr auto             pos           = name_str.find(prefix_others);
    if (pos == std::string_view::npos)
        return name_str; // Can't find prefix

    // Remove function prefix
    constexpr auto wo_prefix = name_str.substr(pos + prefix_others.size());

    // Remove common postfix
    constexpr std::string_view end_name_chars = "];";
    return wo_prefix.substr(0, wo_prefix.find_first_of(end_name_chars));
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
