#pragma once

#include <memory>
#include <optional>
#include <variant>
/**
 * @file variant_utils.hpp
 * @brief This C++ template code is used to work with `std::variant` data structures in a more convenient way by
 * providing helper functions for getting values from a `std::variant` and checking its index.
 *
 * @see https://en.cppreference.com/w/cpp/utility/variant
 */

namespace xsdk::xbase {
/**
 * @brief Template to retrieve the value of a given type T from a variant container of Types...
 * @tparam T The desired type to retrieve from the variant
 * @tparam Types... The types that make up the variant container
 * @param _var_p A pointer to the variant container
 * @return An optional object of type std::optional<T> containing the value of type T from the variant or std::nullopt
 * if T is not present in the variant
 */
template <class T, class... Types>
constexpr std::optional<T> VariantGet(const std::variant<Types...>* _var_p)
{
    const auto* p = std::get_if<T>(_var_p);
    if (!p)
        return std::nullopt;

    return *p;
}

/**
 * @brief Template to determine the index of a specific type TCheck in a variant container TVariant
 * @tparam TVariant The variant container
 * @tparam TCheck The specific type to find in the variant container
 * @tparam TIndex The index of the type in the variant container
 * @return The index of the type TCheck in the variant container TVariant
 */
template <typename TVariant, typename TCheck, std::size_t TIndex = 0>
constexpr std::size_t VariantIndex()
{
    static_assert(std::variant_size_v<TVariant> > TIndex, "type not found in TVariant");
    if constexpr (TIndex == std::variant_size_v<TVariant>)
        return TIndex;
    else if constexpr (std::is_same_v<std::variant_alternative_t<TIndex, TVariant>, TCheck>)
        return TIndex;
    else
        return VariantIndex<TVariant, TCheck, TIndex + 1>();
}
/**
 * @brief A templated helper struct to build a new std::variant type
 * from an existing one and additional types.
 *
 * @tparam T The base variant type.
 * @tparam Args The new types to be appended to the base variant.
 */
template <typename T, typename... Args>
struct VariantAppend;

/**
 * @brief Specialization of VariantAppend for combining a std::variant container of Args0... and Args1...
 * @see VariantAppend
 */
template <typename... Args0, typename... Args1>
struct VariantAppend<std::variant<Args0...>, Args1...> {
    using type = std::variant<Args0..., Args1...>;
};

} // namespace xsdk::xbase