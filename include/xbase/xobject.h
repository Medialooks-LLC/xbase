#pragma once

#include "xpointers.h"
#include "xuid.h"

#include <any>
#include <memory>
#include <string>

namespace xsdk {

/**
 * @brief Interface for objects that can be managed by a smart pointer system.
 * The IObject interface defines a basic set of functions that are required for an object
 * to be managed by a smart pointer system.
 */
class IObject: public xbase::PtrBase<IObject> {

public:
    virtual ~IObject() = default;

    /**
     * @brief Returns a unique identifier for this object.
     * This method must be implemented in derived classes to return a unique identifier for
     * the object.
     * @return The object UID.
     */
    virtual uint64_t ObjectUid() const                       = 0;
    /**
     * @brief Method for querying a pointer to an object of a given type.
     * @param _type_query The UID of the target object type.
     * @return A smart pointer to the queried object or null if not found.
     */
    virtual std::any QueryPtr(xbase::Uid _type_query)        = 0;
    /**
     * @brief Method for querying a pointer to a constant object of a given type.
     * @param _type_query The UID of the target object type.
     * @return A constant smart pointer to the queried object or null if not found.
     */
    virtual std::any QueryPtrC(xbase::Uid _type_query) const = 0;
};

namespace xobject {

    /**
     * @brief Creates a new unique smart pointer for an IObject.
     * @return A new unique smart pointer for an IObject.
     */
    IObject::UPtr CreateUnique();
    /**
     * @brief Creates a new shared smart pointer for an IObject.
     * @return A new shared smart pointer for an IObject.
     */
    IObject::SPtr CreateShared();

    /**
     * @brief Template function for querying a shared_ptr to an object of a given type from an IObject.
     * @tparam TObject The type of the object to query.
     * @param _p_obj The IObject instance.
     * @return The queried shared pointer to the object or nullptr if the object is not present.
     */
    template <typename TObject>
    static std::shared_ptr<TObject> PtrQuery(IObject* _p_obj)
    {
        if (!_p_obj)
            return nullptr;

        auto ptr_any = _p_obj->QueryPtr(xbase::TypeUid<TObject>());
        auto obj_spp = std::any_cast<std::shared_ptr<TObject>>(&ptr_any);
        return obj_spp ? *obj_spp : nullptr;
    }

    /**
     * @brief Template function for querying a shared_ptr to a constant object of a given type from an IObject.
     * @tparam TObject The type of the object to query.
     * @param _p_obj The IObject instance.
     * @return The queried shared pointer to the constant object or nullptr if the object is not present.
     */
    template <typename TObject>
    static std::shared_ptr<const TObject> PtrQuery(const IObject* _p_obj)
    {
        if (!_p_obj)
            return nullptr;

        auto ptr_any = _p_obj->QueryPtrC(xbase::TypeUid<const TObject>());
        auto obj_spp = std::any_cast<std::shared_ptr<const TObject>>(&ptr_any);
        return obj_spp ? *obj_spp : nullptr;
    }

} // namespace xobject
} // namespace xsdk