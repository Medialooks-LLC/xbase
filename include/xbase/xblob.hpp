#pragma once

#include "holder.hpp"
#include "xpointers.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace xsdk::xbase {

/**
 * @struct BlobTyped
 * @tparam TData Type of data which will be stored in container
 * @brief A templated blob structure for storing data of type TData.
 * These C++ templates are called `BlobTyped` and `BlobTypedC`. They are used to create a dynamic, resizable, and
 * copyable container of a given data type `TData`. The `BlobTyped` version of the template can be used with both
 * movable and copyable data types, while the `BlobTypedC` version is specifically designed for working with const data.
 */
template <typename TData>
struct BlobTyped {
    /// @brief The size of the data in bytes.
    size_t size = 0;
    /// @brief A pointer to the data.
    TData* data = nullptr;

public:
    /// @brief Default constructor.
    BlobTyped() = default;
    /// @brief Default move constructor.
    BlobTyped(BlobTyped&&) = default;
    /// @brief Default copy constructor.
    BlobTyped(const BlobTyped&) = default;

    /**
     * @brief Constructor taking size and data.
     * @param _size The size of the data in bytes.
     * @param _data A pointer to the data.
     */
    BlobTyped(const size_t _size, TData* _data = nullptr) : size(_size), data(_data) {}

    /**
     * @brief Constructor taking std::pair.
     * @param _data A std::pair containing the size and data.
     */
    BlobTyped(std::pair<size_t, TData*>&& _data) : size(_data.first), data(_data.second) {}

    /**
     * @brief Constructor taking std::vector.
     * @param _vec_p A pointer to the std::vector.
     */
    BlobTyped(std::vector<TData>* _vec_p) : size(_vec_p ? _vec_p->size() : 0), data(_vec_p ? _vec_p->data() : nullptr)
    {
    }

    BlobTyped& operator=(BlobTyped&&)      = default;
    BlobTyped& operator=(const BlobTyped&) = default;

    /**
     * @brief Converts another templated type to the BlobTyped.
     * @tparam TConvert The target templated type for conversion.
     * @return A BlobTyped with the converted data.
     */
    template <typename TConvert>
    BlobTyped<TConvert> Convert() const
    {
        return {size * sizeof(TData) / sizeof(TConvert), reinterpret_cast<TConvert*>(data)};
    }
};

/**
 * @struct BlobTypedC
 * @tparam TData Type of data which will be stored in container
 * @brief A templated blob structure for storing const data of type TData.
 * These C++ templates are called `BlobTyped` and `BlobTypedC`. They are used to create a dynamic, resizable, and
 * copyable container of a given data type `TData`. The `BlobTyped` version of the template can be used with both
 * movable and copyable data types, while the `BlobTypedC` version is specifically designed for working with const data.
 */
template <typename TData>
struct BlobTypedC {
    /// @brief The size of the data in bytes.
    size_t size = 0;
    /// @brief A constant pointer to the data.
    const TData* data = nullptr;

public:
    /// @brief Default constructor.
    BlobTypedC() = default;
    /// @brief Default move constructor.
    BlobTypedC(BlobTypedC&&) = default;
    /// @brief Default copy constructor.
    BlobTypedC(const BlobTypedC&) = default;

    /**
     * @brief Copy constructor.
     * @param _copy The source to copy from.
     */
    BlobTypedC(const BlobTyped<TData>& _copy) : size(_copy.size), data(_copy.data) {}

    /**
     * @brief Constructor taking std::pair.
     * @param _data A std::pair containing the size and constant data.
     */
    BlobTypedC(std::pair<size_t, const TData*>&& _data) : size(_data.first), data(_data.second) {}

    /**
     * @brief Constructor taking size and data.
     * @param _size The size of the data in bytes.
     * @param _data A constant pointer to the data.
     */
    BlobTypedC(const size_t _size, const TData* _data = nullptr) : size(_size), data(_data) {}

    /**
     * @brief Constructor taking std::vector.
     * @param _vec_p A pointer to the std::vector.
     */
    BlobTypedC(const std::vector<TData>* _vec_p)
        : size(_vec_p ? _vec_p->size() : 0),
          data(_vec_p ? _vec_p->data() : nullptr)
    {
    }

    BlobTypedC& operator=(BlobTypedC&&)      = default;
    BlobTypedC& operator=(const BlobTypedC&) = default;

    /**
     * @brief Converts another templated type to the BlobTypedC.
     * @tparam TConvert The target templated type for conversion.
     * @return A BlobTypedC with the converted data.
     */
    template <typename TConvert>
    BlobTypedC<TConvert> Convert() const
    {
        return {size * sizeof(TData) / sizeof(TConvert), reinterpret_cast<const TConvert*>(data)};
    }
};

/**
 * @typedef XBlob
 * @brief A BlobTyped with uint8_t data type.
 */
using XBlob = BlobTyped<uint8_t>;
/**
 * @typedef XBlobC
 * @brief A BlobTypedC with uint8_t data type.
 */
using XBlobC = BlobTypedC<uint8_t>;

/**
 * @typedef XBlobVoid
 * @brief A BlobTyped with void data type.
 */
using XBlobVoid = BlobTyped<void>;
/**
 * @typedef XBlobVoidC
 * @brief A BlobTypedC with void data type.
 */
using XBlobVoidC = BlobTypedC<void>;

/**
 * @brief A Holder for BlobTyped.
 */
template <typename TData>
using BufferTyped = HolderP<BlobTyped<TData>>;
/**
 * @brief A Holder for BlobTypedC.
 */
template <typename TData>
using BufferTypedC = HolderP<BlobTypedC<TData>>;

/**
 * @typedef XBuffer
 * @brief A @ref BufferTyped with uint8_t data type.
 */
using XBuffer = BufferTyped<uint8_t>;
/**
 * @typedef XBufferC
 * @brief A @ref BufferTypedC with uint8_t data type.
 */
using XBufferC = BufferTypedC<uint8_t>;

/**
 * @brief Allocate unique ptr to vector and optionally copy data into it's
 */
template <typename TData>
std::unique_ptr<std::vector<TData>> VecPtrAlloc(const size_t _size, const TData* _data_p = nullptr)
{
    auto vector_p = std::make_unique<std::vector<TData>>(_size);
    if (_data_p)
        std::memcpy(vector_p->data(), _data_p, sizeof(TData) * _size);

    return vector_p;
}

/**
 * @brief A @ref create and allocate memory for BufferTyped with data type.
 */
template <typename TData>
BufferTyped<TData> BufferCreate(const size_t _size, const TData* _data_p = nullptr)
{
    if (_size == 0)
        return {};

    auto holder_sp = xbase::ToShared(xbase::VecPtrAlloc(_size, _data_p));
    assert(holder_sp);
    return BufferTyped<TData> {BlobTyped<TData> {holder_sp->size(), holder_sp->data()}, holder_sp};
}

/**
 * @brief A @ref create and allocate memory for const BufferTyped with data type.
 */
template <typename TData>
BufferTypedC<TData> BufferCreateC(const size_t _size, const TData* _data_p = nullptr)
{
    return xbase::BufferCreate(_size, _data_p);
}

/**
 * @brief A @ref make BufferTyped from data and holder, for empty holder allocate and copy data
 */
template <typename TData>
BufferTyped<TData> BufferMake(const size_t _size, TData* _data_p, std::any&& _holder)
{
    if (!_holder.has_value())
        return xbase::BufferCreate(_size, _data_p);

    return BufferTyped<TData> {BlobTyped<TData> {_size, _data_p}, std::move(_holder)};
}

/**
 * @brief A @ref cmake BufferTypedC from data and holder, for empty holder allocate and copy data
 */
template <typename TData>
BufferTypedC<TData> BufferMakeC(const size_t _size, const TData* _data_p, std::any&& _holder)
{
    if (!_holder.has_value())
        return xbase::BufferCreateC(_size, _data_p);

    return BufferTypedC<TData> {BlobTypedC<TData> {_size, _data_p}, std::move(_holder)};
}

} // namespace xsdk::xbase
