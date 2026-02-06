#pragma once

#include "xbase.h"

#include <chrono>
#include <cstdint>
#include <thread>

namespace xsdk::xbase::impl {

class BufferImpl final: public IBuffer, public std::enable_shared_from_this<IBuffer> {

    inline static const std::string empty_string_;

    const xbase::Uid uid_;
    using BufferV = std::variant<std::string, BufferTypedC<char>, BufferTypedC<uint8_t>, BufferTyped<uint8_t>>;
    const BufferV       buffer_;
    std::atomic<size_t> actual_size_;

    BufferImpl(const xbase::Uid _uid, BufferV&& _buffer) : uid_(_uid), buffer_(std::move(_buffer))
    {
        assert(_uid != xbase::kInvalidUid);
        actual_size_.store(BufferImpl::BufferCapacity());
    }

public:
    static IBuffer::SPtr Create(BufferV&& _buffer, const std::optional<xbase::Uid> _object_uid);

    virtual uint64_t ObjectUid() const override { return uid_; }
    virtual std::any QueryPtr(xbase::Uid _type_query) override;
    virtual std::any QueryPtrC(xbase::Uid _type_query) const override;

    virtual Type               BufferType() const override;
    virtual const std::string& BufferOwnString() const override;
    virtual std::string_view   BufferData() const override;
    virtual size_t             BufferCapacity() const override;

    virtual uint8_t* BufferWritePtr() override;
    virtual bool     BufferSizeSet(const size_t _new_size) override;
};

} // namespace xsdk::xbase::impl
