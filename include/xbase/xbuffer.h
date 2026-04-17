#pragma once

#include "xobject.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace xsdk {
namespace xbase {

    /**
     * @brief interface for represent srting or binary buffers
     */
    class IBuffer: public IObject {
    public:
        /**
         * @brief buffer type
         */
        enum class Type { kString, kStringView, kBinary, kBinaryWritable };

        USING_PTRS(IBuffer)
    public:
        virtual ~IBuffer() = default;

        /**
         * @brief return buffer type
         */
        virtual Type BufferType() const = 0;
        /**
         * @brief return buffer data for string and binary buffer
         */
        virtual std::string_view BufferData() const = 0;
        /**
         * @brief return buffer data for string and binary buffer
         */
        virtual size_t BufferCapacity() const = 0;
        /**
         * @brief return underliying string ONLY for Type::kString buffers, empty string for other types (including
         * Type::kStringView)
         */
        virtual const std::string& BufferOwnString() const = 0;
        /**
         * @brief return write pointer for writable binary buffer
         */
        virtual uint8_t* BufferWritePtr() = 0;
        /**
         * @brief update buffer size (only for kBinaryWritable ?), return true if size updated,
         * false overwise
         */
        virtual bool BufferSizeSet(const size_t _new_size) = 0;
    };
} // namespace xbase

namespace xbuffer {

    /**
     * @brief check for string buffer (kString or kStringView)
     */
    bool IsString(const xbase::IBuffer* _data_buffer_p);
    /**
     * @brief check for binary buffer (kBinary or kBinaryWritable)
     */
    bool IsBinary(const xbase::IBuffer* _data_buffer_p);

    inline constexpr std::string_view kNull   = "null";
    inline constexpr std::string_view kBinary = "bin";

    /**
     * @brief check for string buffer (kString or kStringView) and return buffer string, or kNull/kBinary if not string
     */
    std::string ToString(const xbase::IBuffer* _data_buffer_p);
    /**
     * @brief create string buffer over std::string
     */
    xbase::IBuffer::SPtr CreateStringBuffer(std::string&& _string, const std::optional<xbase::Uid> _object_uid = {});
    /**
     * @brief create string buffer from std::string_view with holder
     */
    xbase::IBuffer::SPtr CreateStringBuffer(std::any&&                      _holder,
                                            const std::string_view&         _string,
                                            const std::optional<xbase::Uid> _object_uid = {});
    /**
     * @brief create data buffer - for writable buffer keep _holder empty
     */
    xbase::IBuffer::SPtr CreateDataBuffer(const void*                               _data_p,
                                          const size_t                              _size_in_bytes,
                                          std::any&&                                _holder      = {},
                                          const std::optional<xbase::IBuffer::Type> _buffer_type = {},
                                          const std::optional<xbase::Uid>           _object_uid  = {});
} // namespace xbuffer
} // namespace xsdk
