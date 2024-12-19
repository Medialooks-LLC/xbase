#pragma once

#include "xuid.h"

#include <cassert>
#include <memory>

namespace xsdk::xbase {
/**
 * @brief An extensible C++ variant class.
 * The `XAny` class in the provided code is a C++ template designed to implement a generic variant or "any" type. The
 * main purpose of this template is to hold an instance of an object of any given type T, and it provides the
 * functionality to determine the stored type at runtime and safely cast it back to the original type.
 * This template can be particularly useful when dealing with polymorphic or heterogeneous data structures, where
 * storing and working with objects of different types is required. It provides a type-safe way to manipulate objects,
 * allowing safe down-casting and avoiding runtime errors caused by incorrect type casting.
 */
class XAny {

    /**
     * @brief The base class for all instances of XAny.
     */
    class AnyBase {
        const uint64_t type_uid_ = 0;

    public:
        /**
         * @brief Constructor for AnyBase with a given type UID.
         * @param _type_uid The UID of the type represented by this AnyBase instance.
         */
        AnyBase(uint64_t _type_uid) noexcept : type_uid_(_type_uid) {}
        /// @brief Default destructor for AnyBase.
        virtual ~AnyBase() = default;
        /**
         * @brief Get the UID of the type represented by this AnyBase instance.
         * @return The UID of the type.
         */
        uint64_t TypeUid() const noexcept { return type_uid_; }
    };

    /**
     * @tparam T The template type for a specific instance of XAny.
     */
    template <typename T>
    class AnyTyped: public AnyBase {
    public:
        /// @brief The data of the specific instance of XAny.
        T data_;

    public:
        /// @brief Default constructor for AnyTyped.
        AnyTyped(AnyTyped&& _move) = default;
        /**
         * @brief Constructs an AnyTyped instance, moving the data into it.
         * @param _move The data to move into the AnyTyped instance.
         */
        AnyTyped(T&& _move) : AnyBase(xbase::TypeUid<T>()), data_(std::move(_move)) {}
    };

    std::unique_ptr<AnyBase> any_p_;

public:
    /// @brief Default constructor for XAny.
    XAny() = default;
    /// @brief Default move constructor for XAny.
    XAny(XAny&&) = default;
    /// @brief Deleted copy constructor for XAny.
    XAny(const XAny&) = delete;

    /**
     * @brief Constructs an XAny instance, moving the data into it.
     * @tparam T The template type for the data to move into the XAny instance.
     * @param _move The data to move into the XAny instance.
     */
    template <typename T>
    XAny(T&& _move) : any_p_(std::make_unique<AnyTyped<std::decay_t<T>>>(std::forward<T>(_move)))
    {
    }

    /**
     * @brief Gets the UID of the type represented by this XAny instance.
     * @return The UID of the type.
     */
    uint64_t TypeUid() const { return any_p_ ? any_p_->TypeUid() : 0; }

    /**
     * @brief Safely casts this XAny instance to a specific type, if it matches.
     * @tparam T The template type for the desired casted type.
     * @return A pointer to the data of the desired type, or nullptr if the cast fails.
     */
    template <typename T>
    T* AnyCast()
    {
        if (xbase::TypeUid<T>() != TypeUid())
            return nullptr;

        auto* any_typed_p = static_cast<AnyTyped<T>*>(any_p_.get());
        assert(any_typed_p);
        return &any_typed_p->data_;
    }

    /**
     * @brief Const version of AnyCast for const XAny instances.
     * @tparam T The template type for the desired casted type.
     * @return A const pointer to the data of the desired type, or nullptr if the cast fails.
     */
    template <typename T>
    const T* AnyCast() const
    {
        return const_cast<XAny*>(this)->AnyCast<T>();
    }
};

} // namespace xsdk::xbase