#pragma once

#include <memory>

/**
 * @file shared_from_this.hpp
 * @brief Functions to obtain shared or weak pointers from an instance of a template class TClass.
 *
 * These functions allow to obtain smart pointers from instances of a template class TClass,
 * either shared or weak. They use the std::shared_from_this() method which is part of the
 * shared_ptr class, and they perform type safety checks with the help of templates.
 *
 * @tparam TInterface Type of the interface that the TClass implements.
 * @tparam TClass Template class of which an instance is used to obtain the smart pointer.
 * @see https://en.cppreference.com/w/cpp/memory/enable_shared_from_this/shared_from_this
 */
namespace xsdk::xbase {
/**
 * @brief Obtains a shared pointer from an instance of TClass.
 * @tparam TInterface Type of the interface that the TClass implements.
 * @tparam TClass Template class of which an instance is used to obtain the smart pointer.
 * @param _this_p A raw pointer to an instance of the template class TClass.
 * @return A shared pointer to the TClass instance, or nullptr if the type check fails.
 */
template <typename TInterface, class TClass>
std::shared_ptr<TInterface> SharedFromThis(TClass* _this_p)
{
    auto this_sp = std::static_pointer_cast<TClass>(_this_p->weak_from_this().lock());
    if (!this_sp)
        return nullptr; // For ability to add breakpoint

    return std::static_pointer_cast<TInterface>(this_sp);
}

/**
 * @brief Obtains a shared pointer from a constant instance of TClass.
 * @tparam TInterface Type of the interface that the TClass implements.
 * @tparam TClass Template class of which an instance is used to obtain the smart pointer.
 * @param _this_p A const raw pointer to a constant instance of the template class TClass.
 * @return A shared pointer to the constant TClass instance, or nullptr if the type check fails.
 */
template <typename TInterface, class TClass>
std::shared_ptr<const TInterface> SharedFromThis(const TClass* _this_p)
{
    auto this_sp = std::static_pointer_cast<const TClass>(_this_p->weak_from_this().lock());
    if (!this_sp)
        return nullptr; // For ability to add breakpoint

    return std::static_pointer_cast<const TInterface>(this_sp);
}

/**
 * @brief Obtains a weak pointer from an instance of TClass.
 * @tparam TInterface Type of the interface that the TClass implements.
 * @tparam TClass Template class of which an instance is used to obtain the smart pointer.
 * @param _this_p A raw pointer to an instance of the template class TClass.
 * @return A weak pointer to the TClass instance, or nullptr if the type check fails.
 */
template <typename TInterface, class TClass>
std::weak_ptr<TInterface> WeakFromThis(TClass* _this_p)
{
    return SharedFromThis<TInterface>(_this_p);
}

/**
 * @brief Obtains a weak pointer from a constant instance of TClass.
 * @tparam TInterface Type of the interface that the TClass implements.
 * @tparam TClass Template class of which an instance is used to obtain the smart pointer.
 * @param _this_p A const raw pointer to a constant instance of the template class TClass.
 * @return A weak pointer to the constant TClass instance, or nullptr if the type check fails.
 */
template <typename TInterface, class TClass>
std::weak_ptr<const TInterface> WeakFromThis(const TClass* _this_p)
{
    return SharedFromThis<const TInterface>(_this_p);
}

} // namespace xsdk::xbase
