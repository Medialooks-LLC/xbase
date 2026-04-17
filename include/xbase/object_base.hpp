#pragma once

#include "shared_from_this.hpp"
#include "xobject.h"
#include "xuid.h"

#include <any>
#include <memory>
#include <type_traits>

namespace xsdk::xbase {

/* ** Helper for multi-interface QueryPtr/QueryPtrC implementation.
 *
 * TImplClass         - concrete implementation class
 * TPrimaryInterface  - main interface inherited by the object
 * TExtraInterfaces   - additional interfaces supported by QueryPtr
 *
 * Notes:
 * - IObject is supported automatically
 * - TPrimaryInterface is supported automatically
 * - all extra interfaces must be listed explicitly
 */
template <class TImplClass, class TPrimaryInterface, class... TExtraInterfaces>
class ObjectBase: public TPrimaryInterface, public std::enable_shared_from_this<TPrimaryInterface> {
    static_assert(std::is_base_of_v<IObject, TPrimaryInterface>, "TPrimaryInterface must be derived from IObject");

    const xbase::Uid object_uid_ = xbase::kInvalidUid;

public:
    explicit ObjectBase(const xbase::Uid _object_uid) : object_uid_(_object_uid) {}

    virtual ~ObjectBase() = default;

public:
    /* ** IObject */
    virtual uint64_t ObjectUid() const override { return object_uid_; }

    virtual std::any QueryPtr(const xbase::Uid _type_query) override
    {
        if (_type_query == xbase::TypeUid<IObject>())
            return ToAny_(std::static_pointer_cast<IObject>(this->weak_from_this().lock()));

        if (_type_query == xbase::TypeUid<TPrimaryInterface>())
            return ToAny_(std::static_pointer_cast<TPrimaryInterface>(this->weak_from_this().lock()));

        return QueryPtrMany_<TExtraInterfaces...>(_type_query);
    }

    virtual std::any QueryPtrC(const xbase::Uid _type_query) const override
    {
        if (_type_query == xbase::TypeUid<const IObject>())
            return ToAny_(std::static_pointer_cast<const IObject>(this->weak_from_this().lock()));

        if (_type_query == xbase::TypeUid<const TPrimaryInterface>())
            return ToAny_(std::static_pointer_cast<const TPrimaryInterface>(this->weak_from_this().lock()));

        return QueryPtrManyC_<TExtraInterfaces...>(_type_query);
    }

private:
    template <class TInterface>
    static std::any ToAny_(std::shared_ptr<TInterface>&& _interface_sp)
    {
        if (!_interface_sp)
            return {};

        return std::move(_interface_sp);
    }

    template <class TInterface>
    std::any QueryPtrOne_(const xbase::Uid _type_query)
    {
        if (_type_query != xbase::TypeUid<TInterface>())
            return {};

        return ToAny_(std::static_pointer_cast<TInterface>(xbase::SharedFromThis<TImplClass>(this)));
    }

    template <class TInterface>
    std::any QueryPtrOneC_(const xbase::Uid _type_query) const
    {
        if (_type_query != xbase::TypeUid<const TInterface>())
            return {};

        return ToAny_(std::static_pointer_cast<const TInterface>(xbase::SharedFromThis<TImplClass>(this)));
    }

    template <class... TInterfaces>
    std::any QueryPtrMany_(const xbase::Uid _type_query)
    {
        std::any result;
        ((result.has_value() ? void() : void(result = QueryPtrOne_<TInterfaces>(_type_query))), ...);
        return result;
    }

    template <class... TInterfaces>
    std::any QueryPtrManyC_(const xbase::Uid _type_query) const
    {
        std::any result;
        ((result.has_value() ? void() : void(result = QueryPtrOneC_<TInterfaces>(_type_query))), ...);
        return result;
    }
};

} // namespace xsdk::xbase
