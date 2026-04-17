#include "buffer_impl.h"
#include "xbase/variant_utils.hpp"

#include <cassert>

namespace xsdk {

bool xbuffer::IsString(const xbase::IBuffer* _data_buffer_p)
{
    return _data_buffer_p && (_data_buffer_p->BufferType() == xbase::IBuffer::Type::kString ||
                              _data_buffer_p->BufferType() == xbase::IBuffer::Type::kStringView);
}

bool xbuffer::IsBinary(const xbase::IBuffer* _data_buffer_p)
{
    return _data_buffer_p && (_data_buffer_p->BufferType() == xbase::IBuffer::Type::kBinary ||
                              _data_buffer_p->BufferType() == xbase::IBuffer::Type::kBinaryWritable);
}

std::string xbuffer::ToString(const xbase::IBuffer* _data_buffer_p)
{
    if (!_data_buffer_p)
        return std::string {xbuffer::kNull};

    if (!xbuffer::IsString(_data_buffer_p))
        return std::string {xbuffer::kBinary}; // 2Think: return unique key as for RPC?

    return std::string(_data_buffer_p->BufferData());
}

xbase::IBuffer::SPtr xbuffer::CreateStringBuffer(std::string&& _string, const std::optional<xbase::Uid> _object_uid)
{
    return xbase::impl::BufferImpl::Create(std::move(_string), _object_uid);
}

xbase::IBuffer::SPtr xbuffer::CreateStringBuffer(std::any&&                      _holder,
                                                 const std::string_view&         _string_view,
                                                 const std::optional<xbase::Uid> _object_uid)
{
    if (!_holder.has_value())
        return xbase::impl::BufferImpl::Create(std::string(_string_view), _object_uid);

    return xbase::impl::BufferImpl::Create(
        xbase::BufferMakeC(_string_view.size(), _string_view.data(), std::move(_holder)),
        _object_uid);
}

xbase::IBuffer::SPtr xbuffer::CreateDataBuffer(const void*                               _data_p,
                                               const size_t                              _size_in_bytes,
                                               std::any&&                                _holder,
                                               const std::optional<xbase::IBuffer::Type> _buffer_type,
                                               const std::optional<xbase::Uid>           _object_uid)
{
    auto buffer_type = _buffer_type.value_or(_holder.has_value() ? xbase::IBuffer::Type::kBinary :
                                                                   xbase::IBuffer::Type::kBinaryWritable);
    if (buffer_type == xbase::IBuffer::Type::kBinary) {
        return xbase::impl::BufferImpl::Create(
            xbase::BufferMakeC(_size_in_bytes, (const uint8_t*)_data_p, std::move(_holder)),
            _object_uid);
    }

    if (buffer_type == xbase::IBuffer::Type::kBinaryWritable) {
        return xbase::impl::BufferImpl::Create(xbase::BufferMake(_size_in_bytes, (uint8_t*)_data_p, std::move(_holder)),
                                               _object_uid);
    }

    if (buffer_type == xbase::IBuffer::Type::kString) {
        assert(!_holder.has_value());
        return xbuffer::CreateStringBuffer(std::string((const char*)_data_p, _size_in_bytes), _object_uid);
    }

    assert(buffer_type == xbase::IBuffer::Type::kStringView);
    return xbuffer::CreateStringBuffer(std::move(_holder),
                                       std::string_view((const char*)_data_p, _size_in_bytes),
                                       _object_uid);
}

namespace xbase::impl {

    BufferImpl::BufferImpl(const xbase::Uid _uid, BufferV&& _buffer) : ObjectBase_(_uid), buffer_(std::move(_buffer))
    {
        assert(_uid != xbase::kInvalidUid);
        actual_size_.store(BufferImpl::BufferCapacity());
    }

    IBuffer::SPtr BufferImpl::Create(BufferV&& _buffer, const std::optional<xbase::Uid> _object_uid)
    {
        // Do not use .value_or(xbase::NextUid()) for do not alloc uid even if _object_uid valid
        auto object_uid = _object_uid.value_or(xbase::kInvalidUid);
        if (object_uid == xbase::kInvalidUid)
            object_uid = xbase::NextUid();

        std::shared_ptr<BufferImpl> buffer_p {new BufferImpl(object_uid, std::move(_buffer))};
        return buffer_p;
    }

    IBuffer::Type BufferImpl::BufferType() const
    {
        switch (buffer_.index()) {
            case VariantIndex<BufferV, std::string>():
                return Type::kString;
            case VariantIndex<BufferV, BufferTypedC<char>>():
                return Type::kStringView;
            case VariantIndex<BufferV, BufferTypedC<uint8_t>>():
                return Type::kBinary;
            case VariantIndex<BufferV, BufferTyped<uint8_t>>():
                return Type::kBinaryWritable;
        }

        assert("!BufferType - wrong varaint state");
        return {};
    }

    const std::string& BufferImpl::BufferOwnString() const
    {
        const auto* str_p = std::get_if<std::string>(&buffer_);
        if (!str_p) {
            static const std::string empty_string;
            return empty_string;
        }

        return *str_p;
    }

    std::string_view BufferImpl::BufferData() const
    {
        const auto* u8c_data_p = std::get_if<BufferTypedC<uint8_t>>(&buffer_);
        if (u8c_data_p)
            return std::string_view {(const char*)u8c_data_p->DataPtr()->data, u8c_data_p->DataPtr()->size};

        const auto* char_data_p = std::get_if<BufferTypedC<char>>(&buffer_);
        if (char_data_p)
            return std::string_view {char_data_p->DataPtr()->data, char_data_p->DataPtr()->size};

        const auto* u8_data_p = std::get_if<BufferTyped<uint8_t>>(&buffer_);
        if (u8_data_p)
            return std::string_view {(const char*)u8_data_p->DataPtr()->data, actual_size_.load()};

        const auto* str_p = std::get_if<std::string>(&buffer_);
        assert(str_p && "BufferData");
        return *str_p;
    }

    size_t BufferImpl::BufferCapacity() const
    {
        const auto* u8_data_p = std::get_if<BufferTyped<uint8_t>>(&buffer_);
        if (u8_data_p)
            return u8_data_p->DataPtr()->size;

        const auto* char_data_p = std::get_if<BufferTypedC<char>>(&buffer_);
        if (char_data_p)
            return char_data_p->DataPtr()->size;

        const auto* u8c_data_p = std::get_if<BufferTypedC<uint8_t>>(&buffer_);
        if (u8c_data_p)
            return u8c_data_p->DataPtr()->size;

        const auto* str_p = std::get_if<std::string>(&buffer_);
        assert(str_p && "BufferCapacity ");
        return str_p->size();
    }

    uint8_t* BufferImpl::BufferWritePtr()
    {
        const auto* u8_data_p = std::get_if<BufferTyped<uint8_t>>(&buffer_);
        if (u8_data_p)
            return u8_data_p->DataPtr()->data;

        return nullptr;
    }

    bool BufferImpl::BufferSizeSet(const size_t _new_size)
    {
        auto* u8_data_p = std::get_if<BufferTyped<uint8_t>>(&buffer_);
        if (!u8_data_p || _new_size > u8_data_p->DataPtr()->size)
            return false;

        actual_size_.store(_new_size);
        return true;
    }
} // namespace xbase::impl
} // namespace xsdk