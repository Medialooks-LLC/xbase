#include "xbase/vectors.hpp"

namespace xsdk::xvector {

size_t WriteBytes(const xbase::XBlob _dst, PFOnWrite<uint8_t>&& _on_write_pf, const size_t _offset)
{
    return Write<uint8_t>(_dst, std::move(_on_write_pf), _offset);
}

size_t ReadBytes(const xbase::XBlobC _src,
                 std::function<int64_t(const xbase::XBlobC&, size_t)>&& _on_read_pf,
                 const size_t _offset)
{
    return Read<uint8_t>(_src, std::move(_on_read_pf), _offset);
}

} // namespace xsdk::xvector
