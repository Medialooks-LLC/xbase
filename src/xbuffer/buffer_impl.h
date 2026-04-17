#pragma once

#include "xbase/xblob.hpp"
#include "xbase/xbuffer.h"
#include "xbase/object_base.hpp"

#include <cstdint>
#include <variant>
#include <atomic>

namespace xsdk::xbase::impl {

class BufferImpl final: public xbase::ObjectBase<BufferImpl, IBuffer> {

    using ObjectBase_ = xbase::ObjectBase<BufferImpl, IBuffer>;
    using BufferV     = std::variant<std::string, BufferTypedC<char>, BufferTypedC<uint8_t>, BufferTyped<uint8_t>>;

private:
    const BufferV       buffer_;
    std::atomic<size_t> actual_size_;

private:
    BufferImpl(const xbase::Uid _uid, BufferV&& _buffer);

public:
    static IBuffer::SPtr Create(BufferV&& _buffer, const std::optional<xbase::Uid> _object_uid);

    virtual Type               BufferType() const override;
    virtual const std::string& BufferOwnString() const override;
    virtual std::string_view   BufferData() const override;
    virtual size_t             BufferCapacity() const override;

    virtual uint8_t* BufferWritePtr() override;
    virtual bool     BufferSizeSet(const size_t _new_size) override;
};

} // namespace xsdk::xbase::impl
