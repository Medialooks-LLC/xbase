#pragma once

#include "xbase/numbers.hpp"
#include "xbase/xblob.hpp"

#include <algorithm>
#include <any>
#include <cassert>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace xsdk::xvector {

template <typename T>
using SPtr = std::shared_ptr<std::vector<T>>;

template <typename T>
using SPtrC = std::shared_ptr<const std::vector<T>>;

template <typename T>
using UPtr = std::unique_ptr<std::vector<T>>;

template <typename T>
using UPtrC = std::unique_ptr<const std::vector<T>>;

template <typename TData>
using PFOnWrite = std::function<int64_t(const xbase::BlobTyped<TData>&, size_t)>;

template <typename TData>
std::vector<TData> AllocWrite(const size_t _count, PFOnWrite<TData>&& _on_write_pf, const size_t _reserve = 0)
{
    std::vector<TData> result;
    result.reserve(std::max(_count, _reserve));
    result.resize(_count);

    int64_t pos = 0;
    while (pos < static_cast<int64_t>(result.size())) {
        const auto offset = static_cast<size_t>(pos);
        const int64_t move =
            _on_write_pf(xbase::BlobTyped<TData> {result.size() - offset, result.data() + offset}, offset);
        if (!move || pos + move < 0)
            break;

        pos += std::max(-pos, move);
    }

    return result;
}

template <typename TData>
std::vector<TData> Alloc(const size_t _data_count, const void* const _data_p, const size_t _reserve = 0)
{
    static_assert(std::is_trivially_copyable_v<TData>, "TData must be trivially copyable.");

    std::vector<TData> result;
    result.reserve(std::max(_data_count, _reserve));
    result.resize(_data_count);

    if (_data_p)
        std::memcpy(result.data(), _data_p, _data_count * sizeof(TData));

    return result;
}

template <typename TData>
std::vector<TData> Alloc(const xbase::BlobTypedC<TData> _src, const size_t _reserve = 0)
{
    return Alloc<TData>(_src.size, _src.data, _reserve);
}

template <typename TData>
UPtr<TData> UPtrAlloc(const size_t _data_count, const void* const _data_p = nullptr, const size_t _reserve = 0)
{
    return std::make_unique<std::vector<TData>>(Alloc<TData>(_data_count, _data_p, _reserve));
}

template <typename TData>
UPtr<TData> UPtrAlloc(const xbase::BlobTypedC<TData> _src, const size_t _reserve = 0)
{
    return UPtrAlloc<TData>(_src.size, _src.data, _reserve);
}

template <typename TData>
UPtr<TData> UPtrAllocWrite(const size_t _count, PFOnWrite<TData>&& _on_write_pf, const size_t _reserve = 0)
{
    return std::make_unique<std::vector<TData>>(AllocWrite<TData>(_count, std::move(_on_write_pf), _reserve));
}

template <typename TData>
UPtr<TData> UPtrWrap(std::vector<TData>&& _data)
{
    return std::make_unique<std::vector<TData>>(std::move(_data));
}

template <typename TData>
SPtr<TData> SPtrAlloc(const size_t _data_count, const void* const _data_p = nullptr, const size_t _reserve = 0)
{
    return std::make_shared<std::vector<TData>>(Alloc<TData>(_data_count, _data_p, _reserve));
}

template <typename TData>
SPtr<TData> SPtrAlloc(const xbase::BlobTypedC<TData> _src, const size_t _reserve = 0)
{
    return SPtrAlloc<TData>(_src.size, _src.data, _reserve);
}

template <typename TData>
SPtr<TData> SPtrAllocWrite(const size_t _count, PFOnWrite<TData>&& _on_write_pf, const size_t _reserve = 0)
{
    return std::make_shared<std::vector<TData>>(AllocWrite<TData>(_count, std::move(_on_write_pf), _reserve));
}

template <typename TData>
SPtr<TData> SPtrWrap(std::vector<TData>&& _data)
{
    return std::make_shared<std::vector<TData>>(std::move(_data));
}

template <typename TData>
size_t Write(const xbase::BlobTyped<TData> _dst, PFOnWrite<TData>&& _on_write_pf, const size_t _offset = 0)
{
    auto pos = static_cast<int64_t>(_offset);
    while (pos < static_cast<int64_t>(_dst.size)) {
        const auto offset = static_cast<size_t>(pos);
        const int64_t move =
            _on_write_pf(xbase::BlobTyped<TData> {_dst.size - offset, _dst.data + offset}, offset);
        if (!move)
            break;

        pos += std::max(-pos, move);
    }
    return static_cast<size_t>(std::max<int64_t>(0, pos));
}

size_t WriteBytes(xbase::XBlob _dst, PFOnWrite<uint8_t>&& _on_write_pf, size_t _offset);

template <typename TData>
size_t Read(const xbase::BlobTypedC<TData>                                   _src,
            std::function<int64_t(const xbase::BlobTypedC<TData>&, size_t)>&& _on_read_pf,
            const size_t                                                      _offset = 0)
{
    auto pos = static_cast<int64_t>(_offset);
    while (pos < static_cast<int64_t>(_src.size)) {
        const auto offset = static_cast<size_t>(pos);
        const int64_t move =
            _on_read_pf(xbase::BlobTypedC<TData> {_src.size - offset, _src.data + offset}, offset);
        if (!move)
            break;

        pos += std::max(-pos, move);
    }
    return static_cast<size_t>(std::max<int64_t>(0, pos));
}

size_t ReadBytes(xbase::XBlobC _src,
                 std::function<int64_t(const xbase::XBlobC&, size_t)>&& _on_read_pf,
                 size_t _offset);

template <typename TData>
xbase::BufferTyped<TData> MakeBuffer(const xbase::BlobTyped<TData> _src, std::any&& _holder = {})
{
    if (_holder.has_value() || _src.size == 0)
        return xbase::BufferTyped<TData>(_src, std::move(_holder));

    auto buffer_p = SPtrAlloc<TData>(_src);
    return xbase::BufferTyped<TData>(buffer_p.get(), buffer_p);
}

template <typename TData>
xbase::BufferTypedC<TData> MakeBufferC(const xbase::BlobTypedC<TData> _src, std::any&& _holder = {})
{
    if (_holder.has_value() || _src.size == 0)
        return xbase::BufferTypedC<TData>(_src, std::move(_holder));

    auto buffer_p = SPtrAlloc<TData>(_src);
    return xbase::BufferTypedC<TData>(buffer_p.get(), buffer_p);
}

template <typename T>
std::vector<T> Intersection(const std::vector<T>& _first, const std::vector<T>& _second)
{
    std::vector<T> result;
    for (const auto& value : _first) {
        if (std::find(_second.begin(), _second.end(), value) != _second.end())
            result.push_back(value);
    }

    return result;
}

template <typename T>
std::vector<T> Intersection(const std::vector<T>* const _first_p, const std::vector<T>* const _second_p)
{
    if (!_first_p || !_second_p)
        return {};

    return Intersection(*_first_p, *_second_p);
}

template <typename T>
std::vector<T> SmartIntersection(const std::vector<T>& _required, const std::vector<T>& _selected)
{
    if (_required.empty() || _selected.empty())
        return _required.empty() ? _selected : _required;

    auto intersection = Intersection(_required, _selected);
    if (intersection.empty())
        return _required;

    return intersection;
}

template <typename T>
std::string Dump(const std::vector<T>& _data_vec, const std::string_view _delimiter = ",")
{
    std::string result;
    for (const auto& value : _data_vec) {
        if (!result.empty())
            result += _delimiter;

        result += std::to_string(value);
    }

    return result;
}

inline std::string Dump(const std::vector<std::string>& _data_vec,
                        const bool                      _ignore_empty,
                        const std::string_view          _delimiter = ",")
{
    std::string result;
    for (const auto& value : _data_vec) {
        if (_ignore_empty && value.empty())
            continue;

        if (!result.empty())
            result += _delimiter;

        result += value;
    }

    return result;
}

template <typename T>
int32_t Compare(const std::vector<T>* const _left_p, const std::vector<T>* const _right_p)
{
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable.");

    if (!_left_p || !_right_p)
        return _left_p ? 1 : _right_p ? -1 : 0;

    if (_left_p->size() != _right_p->size())
        return _left_p->size() < _right_p->size() ? -1 : 1;

    return std::memcmp(_left_p->data(), _right_p->data(), _left_p->size() * sizeof(T));
}

template <typename T>
int32_t Compare(const xbase::BlobTypedC<T> _left, const xbase::BlobTypedC<T> _right)
{
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable.");

    if (!_left.data || !_right.data)
        return _left.data ? 1 : _right.data ? -1 : 0;

    if (_left.size != _right.size)
        return _left.size < _right.size ? -1 : 1;

    return std::memcmp(_left.data, _right.data, _left.size * sizeof(T));
}

template <typename TData>
size_t AddMul(std::vector<TData>&       _dst,
              const size_t              _offset,
              const std::vector<TData>& _add,
              const size_t              _count,
              const TData&              _multiply)
{
    if (_offset >= _dst.size())
        return 0;

    auto*       dst_p = _dst.data() + _offset;
    const auto* add_p = _add.data();
    const auto  count = std::min(_dst.size() - _offset, std::min(_add.size(), _count));

    if (_multiply == 1) {
        for (size_t z = 0; z < count; ++z)
            dst_p[z] = dst_p[z] + add_p[z];
    }
    else if (_multiply == -1) {
        for (size_t z = 0; z < count; ++z)
            dst_p[z] = dst_p[z] - add_p[z];
    }
    else {
        for (size_t z = 0; z < count; ++z)
            dst_p[z] = dst_p[z] + add_p[z] * _multiply;
    }

    return count;
}

template <typename TDst, typename TSrc>
UPtr<TDst> Convert(const TSrc* const _data_p,
                   const size_t      _count,
                   const double      _multiply_by = 1.0,
                   const bool        _clip_value  = true)
{
    auto  data_p = UPtrAlloc<TDst>(_count);
    auto* dst_p  = data_p->data();
    if (_clip_value) {
        for (size_t z = 0; z < _count; ++z)
            dst_p[z] = xbase::Clamp<TDst>(_data_p[z] * _multiply_by);
    }
    else {
        for (size_t z = 0; z < _count; ++z)
            dst_p[z] = static_cast<TDst>(_data_p[z] * _multiply_by);
    }
    return data_p;
}

template <typename T>
size_t Append(std::vector<T>& _dest, const std::vector<T>& _src)
{
    if (_dest.empty()) {
        _dest = _src;
        return _dest.size();
    }

    _dest.insert(std::end(_dest), std::cbegin(_src), std::cend(_src));
    return _src.size();
}

template <typename T>
size_t Append(std::vector<T>& _dest, std::vector<T>&& _src)
{
    if (_dest.empty()) {
        _dest = std::move(_src);
        return _dest.size();
    }

    const size_t appended = _src.size();
    _dest.insert(std::end(_dest), std::make_move_iterator(std::begin(_src)), std::make_move_iterator(std::end(_src)));
    _src.clear();
    _src.shrink_to_fit();

    return appended;
}

template <class TSample>
TSample* Interleave(const TSample** const _channels_pp,
                    const size_t          _channels_count,
                    const size_t          _channel_samples,
                    TSample* const        _interleaved_p)
{
    assert(_channels_pp && _interleaved_p);
    auto* dest_p = _interleaved_p;
    for (size_t z = 0; z < _channel_samples; ++z) {
        for (size_t ch = 0; ch < _channels_count; ++ch)
            *dest_p++ = _channels_pp[ch][z];
    }

    return _interleaved_p;
}

} // namespace xsdk::xvector
