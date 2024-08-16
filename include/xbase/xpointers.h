#pragma once

#include <memory>

namespace xsdk::xbase {

/**
 * @brief Base class template for managing smart pointers of a derived class TDerived
 *
 * This base class template provides aliases for different types of smart pointers
 * (unique, shared and weak) for the derived class TDerived.
 */

/**
 * @tparam TDerived Type of the derived class
 */
template <class TDerived>
class PtrBase {
public:
    /**
     * @brief Alias for std::shared_ptr of TDerived
     */
    using SPtr  = std::shared_ptr<TDerived>;
    /**
     * @brief Alias for std::shared_ptr of const TDerived
     */
    using SPtrC = std::shared_ptr<const TDerived>;

    /**
     * @brief Alias for std::unique_ptr of TDerived
     */
    using UPtr  = std::unique_ptr<TDerived>;
    /**
     * @brief Alias for std::unique_ptr of const TDerived
     */
    using UPtrC = std::unique_ptr<const TDerived>;
    /**
     * @brief Alias for std::weak_ptr of TDerived
     */
    using WPtr  = std::weak_ptr<TDerived>;
    /**
     * @brief Alias for std::weak_ptr of const TDerived
     */
    using WPtrC = std::weak_ptr<const TDerived>;
};

// For derived class
/**
 * @brief Macro for derived classes to use the smart pointers defined in PtrBase
 *
 * @param _class Type of the derived class
 */
#define USING_PTRS(_class)                       \
    /** @brief Alias for std::unique_ptr of _class */ \
    using UPtr  = std::unique_ptr<_class>;       \
    /** @brief Alias for std::shared_ptr of _class */ \
    using SPtr  = std::shared_ptr<_class>;       \
    /** @brief Alias for std::weak_ptr of _class */ \
    using WPtr  = std::weak_ptr<_class>;         \
    /** @brief Alias for std::unique_ptr of const _class */ \
    using UPtrC = std::unique_ptr<const _class>; \
    /** @brief Alias for std::shared_ptr of const _class */ \
    using SPtrC = std::shared_ptr<const _class>; \
    /** @brief Alias for std::weak_ptr of const _class */ \
    using WPtrC = std::weak_ptr<const _class>;


} // namespace xsdk::xbase
