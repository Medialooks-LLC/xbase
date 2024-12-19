#pragma once

#include "holder.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
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

} // namespace xsdk::xbase
