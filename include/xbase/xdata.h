#pragma once

#include "xuid.h" // For TypeUid
#include "xpointers.h"

#include <any>
#include <cassert>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace xsdk {

/**
 * @brief Interface for data container.
 *
 * This interface provides methods for data handling, such as data adding, data getting, data removing, and data
 * resetting.
 */
class IData: public xbase::PtrBase<IData> {
public:
    virtual ~IData() = default;

    /**
     * @brief Enum class for CloneSetType.
     *
     * Defines the include or exclude behavior when cloning data.
     */
    enum class CloneSetType { Include, Exclude };
    /**
     * @brief Clone an object with necessary types.
     * @param _cloned_types Set of types to include or exclude when cloning.
     * @param _set_type Type of set to use. Exclude or include types from _cloned_types set.
     * @return New cloned object or a null pointer if cloning failed.
     */
    virtual IData::UPtr Clone(const std::set<uint64_t>& _cloned_types = {},
                              CloneSetType              _set_type     = CloneSetType::Exclude) const = 0;
    /**
     * @brief Add or update an entry in the data set of an IData object.
     * @param _data_uid unique identifier for the data set entry
     * @param _face data to be added or updated
     * @param _holder associated object or data to be added or updated
     * @param _idx index of the entry in the data set
     * @return size_t index of the updated entry in the data set
     */
    virtual size_t DataSet(uint64_t _data_uid, std::any&& _face, std::any&& _holder = std::any(), size_t _idx = 0) = 0;
    /**
     * @brief Get a items count by UID.
     * @param _data_uid unique identifier for the data set entry
     * @return Count of items.
     */
    virtual size_t DataCount(uint64_t _data_uid) const                                                             = 0;
    /**
     * @brief Returns the value of the specified index from the data container of the given UID.
     * @param _data_uid unique identifier for the data set entry
     * @param _idx The index of the value to retrieve.
     * @return A std::pair containing the requested value and an empty std::any representing the error case.
     */
    virtual std::pair<std::any, std::any> DataGet(uint64_t _data_uid, size_t _idx = 0) const                       = 0;
    /**
     * @brief Remove an element from the specified index from the data container of the given UID.
     * @param _data_uid unique identifier for the data set entry
     * @param _idx Index of the element to be removed
     * @return the removed element as std::pair
     */
    virtual std::pair<std::any, std::any> DataRemove(uint64_t _data_uid, size_t _idx = 0)                          = 0;
    /**
     * @brief Remove all data from the data container by the given UID.
     * @param _data_uid unique identifier for the data set entry
     * @return True if something has been deleted, otherwise false
     */
    virtual bool                          DataReset(uint64_t _data_uid)                                            = 0;
};

namespace xdata {

/**
 * @brief Creates an empty XData
 * @return std::unique_ptr to the newly created XData
 */
IData::UPtr Create(); // Implemetation in xdata_impl.cpp

/**
 * @brief Helper function for wrapping a data instance in an std::any.
 * @tparam TData The data type to wrap.
 * @param _data Data instance to wrap.
 * @return The wrapped data instance.
 */
template <typename TData>
std::any AnyWrap(TData&& _data)
{
    return std::make_shared<std::decay_t<TData>>(std::forward<TData>(_data));
}

/**
 * @brief Helper function for unwrapping a data instance from an std::any.
 * @tparam TData The data type to unwrap.
 * @param _data The std::any instance to unwrap.
 * @return The unwrapped data instance or a null pointer if unwrapping failed.
 */
template <typename TData>
const TData* AnyUnwrap(const std::any& _data)
{
    const auto* data_pp = std::any_cast<std::shared_ptr<TData>>(&_data);
    return data_pp ? data_pp->get() : nullptr;
}

// Use xdata::Set(data_p, -1, "123") for add new data, return index of added data
/**
 * @brief Set a single data item.
 * @tparam TFace The data type to set.
 * @param _xdata_p Pointer to the IData instance.
 * @param _idx Index for the data to set.
 * @param _face Data instance to set.
 * @return Index of the added data if successful, otherwise -1.
 */
template <typename TFace>
size_t Set(IData* _xdata_p, size_t _idx, TFace&& _face)
{
    if (!_xdata_p)
        return -1;

    using Face = std::decay_t<TFace>;
    return _xdata_p->DataSet(xbase::TypeUid<Face>(), std::make_shared<Face>(std::forward<TFace>(_face)), {}, _idx);
}
/**
 * @brief Set a single data item with an additional holder.
 * @tparam TFace The data type to set.
 * @param _xdata_p Pointer to the IData instance.
 * @param _idx Index for the data to set.
 * @param _face Data instance to set.
 * @param _holder The holder for the data.
 * @return Index of the added data if successful, otherwise -1.
 */
template <typename TFace>
size_t Set(IData* _xdata_p, size_t _idx, TFace&& _face, std::any&& _holder)
{
    if (!_xdata_p)
        return -1;

    using Face = std::decay_t<TFace>;
    return _xdata_p->DataSet(xbase::TypeUid<Face>(),
                             std::make_shared<Face>(std::forward<TFace>(_face)),
                             std::move(_holder),
                             _idx);
}

/**
 * @brief Set a single data item with an additional holder.
 * @tparam TFace The data type to set.
 * @tparam THolder The holder type for the data.
 * @param _xdata_p Pointer to the IData instance.
 * @param _idx Index for the data to set.
 * @param _face Data instance to set.
 * @param _holder The holder for the data.
 * @return Index of the added data if successful, otherwise -1.
 */
template <typename TFace, typename THolder>
size_t Set(IData* _xdata_p, size_t _idx, TFace&& _face, THolder&& _holder)
{
    if (!_xdata_p)
        return -1;

    using Face   = std::decay_t<TFace>;
    using Holder = std::decay_t<THolder>;
    return _xdata_p->DataSet(xbase::TypeUid<Face>(),
                             std::make_shared<Face>(std::forward<TFace>(_face)),
                             std::make_shared<Holder>(std::forward<THolder>(_holder)),
                             _idx);
}

/**
 * @brief Get count of elements of the same type.
 * @tparam TFace The data type to get the count for.
 * @param _xdata_p Pointer to the IData instance.
 * @return Data count if successful, otherwise 0.
 */
template <typename TFace>
size_t Count(const IData* _xdata_p)
{
    if (!_xdata_p)
        return 0;

    return _xdata_p->DataCount(xbase::TypeUid<std::decay_t<TFace>>());
}

/**
 * @brief Get a item by index and its type data type.
 * @tparam TFace The data type to get.
 * @param _xdata_p Pointer to the IData instance.
 * @param _idx Index for the data to get.
 * @param[out] _holder_get A pointer to the holder to be filled with the holder, if any.
 * @return A containing the data if successful, otherwise a null pointer.
 */
template <typename TFace>
std::shared_ptr<const TFace> Get(const IData* _xdata_p, size_t _idx = 0, std::any* _holder_get = nullptr)
{
    if (!_xdata_p)
        return nullptr;

    auto [face, holder] = _xdata_p->DataGet(xbase::TypeUid<TFace>(), _idx);
    const auto* face_pp = std::any_cast<std::shared_ptr<TFace>>(&face);
    if (!face_pp)
        return nullptr;

    if (_holder_get)
        *_holder_get = std::move(holder);

    return *face_pp;
}
/**
 * @brief Get a copy of item which was retrieved by index and its type data type.
 * @tparam TFace The data type to get and copy.
 * @param _xdata_p Pointer to the IData instance.
 * @param _idx Index for the data to get and copy.
 * @param[out] _holder_get A pointer to the holder to be filled with the holder, if any.
 * @return A copy of the data if successful, otherwise a default value.
 */
template <typename TFace>
TFace GetCopy(const IData* _xdata_p, size_t _idx = 0, std::any* _holder_get = nullptr)
{
    auto data_p = Get<TFace>(_xdata_p, _idx, _holder_get);
    if (!data_p)
        return {};

    return *data_p;
}
/**
 * @brief Get all elements of the same type as a vector.
 * @tparam TFace The data type to get the vector for.
 * @param _xdata_p Pointer to the IData instance.
 * @param[out] _holder_get A pointer to the holder to be filled with the holder, if any.
 * @return Vector with data items if successful, otherwise empty vector.
 */
template <typename TFace>
std::vector<TFace> GetCopyVec(const IData* _xdata_p, std::any* _holder_get = nullptr)
{
    auto data_count = Count<TFace>(_xdata_p);
    if (!data_count)
        return {};

    std::vector<TFace> data_vec;
    data_vec.reserve(data_count);
    for (size_t z = 0; z < data_count; ++z) {
        auto data_p = Get<TFace>(_xdata_p, z, (_holder_get && !_holder_get->has_value()) ? _holder_get : nullptr);
        assert(data_p);
        if (data_p)
            data_vec.push_back(*data_p);
    }
    return data_vec;
}

/**
 * @brief Get a item by index and its type data type.
 * @tparam TFace The data type to get.
 * @tparam THolder The holder type for the data.
 * @param _xdata_p Pointer to the IData instance.
 * @param _idx Index for the data to get.
 * @return A containing the data if successful, otherwise a null pointer.
 */
template <typename TFace, typename THolder>
std::pair<std::shared_ptr<const TFace>, std::shared_ptr<const THolder>> GetWithHolder(const IData* _xdata_p,
                                                                                      size_t              _idx = 0)
{
    if (!_xdata_p)
        return {};

    auto [face, holder]   = _xdata_p->DataGet(xbase::TypeUid<TFace>(), _idx);
    const auto* face_pp   = std::any_cast<std::shared_ptr<TFace>>(&face);
    const auto* holder_pp = std::any_cast<std::shared_ptr<THolder>>(&holder);
    return {face_pp ? *face_pp : nullptr, holder_pp ? *holder_pp : nullptr};
}


} // namespace xdata
} // namespace xsdk