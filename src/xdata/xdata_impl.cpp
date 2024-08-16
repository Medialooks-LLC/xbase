#include "xdata_impl.h"

namespace xsdk {

IData::UPtr xdata::Create() { return IData::UPtr {new impl::XDataImpl()}; }

namespace impl {

XDataImpl::XDataImpl(std::map<uint64_t, data_entry>&& _data_map)
    : uid_(xbase::NextUid()),
      data_map_(std::move(_data_map))
{
}

IData::UPtr XDataImpl::Clone(const std::set<uint64_t>& _cloned_types, CloneSetType _set_type) const
{
    //std::shared_lock lck(map_rw_);

    std::map<uint64_t, data_entry> cloned_map;
    if (!_cloned_types.empty()) {
        if (_set_type == CloneSetType::Exclude) {
            for (const auto& [type, val] : data_map_) {
                if (_cloned_types.find(type) != _cloned_types.end())
                    continue;

                cloned_map.emplace(type, val);
            }
        }
        else {
            assert(_set_type == CloneSetType::Include);

            for (const auto& type_uid : _cloned_types) {
                auto it = data_map_.find(type_uid);
                if (it != data_map_.end())
                    cloned_map.emplace(*it);
            }
        }
    }
    else if (_set_type == CloneSetType::Exclude) {
        cloned_map.insert(data_map_.begin(), data_map_.end());
    }

    return IData::UPtr {new XDataImpl(std::move(cloned_map))};
}

size_t XDataImpl::DataSet(uint64_t _data_uid, std::any&& _face, std::any&& _holder, size_t _idx)
{
    //std::unique_lock lck(map_rw_);

    auto it = data_map_.find(_data_uid);
    if (it == data_map_.end())
        it = data_map_.emplace(_data_uid, data_entry()).first;

    if (_idx >= it->second.size()) {
        it->second.emplace_back(std::move(_face), std::move(_holder));
        return it->second.size() - 1;
    }
    it->second[_idx].first  = std::move(_face);
    it->second[_idx].second = std::move(_holder);
    return _idx;
}

size_t XDataImpl::DataCount(uint64_t _data_uid) const
{
    //std::shared_lock lck(map_rw_);

    auto it = data_map_.find(_data_uid);
    return it == data_map_.end() ? 0 : it->second.size();
}

std::pair<std::any, std::any> XDataImpl::DataGet(uint64_t _data_uid, size_t _idx) const
{
    //std::shared_lock lck(map_rw_);

    auto it = data_map_.find(_data_uid);
    if (it == data_map_.end() || _idx >= it->second.size())
        return {};

    return it->second[_idx];
}

std::pair<std::any, std::any> XDataImpl::DataRemove(uint64_t _data_uid, size_t _idx)
{
    //std::unique_lock lck(map_rw_);

    auto it = data_map_.find(_data_uid);
    if (it == data_map_.end() || _idx >= it->second.size())
        return {};

    auto removed = std::move(it->second[_idx]);
    it->second.erase(it->second.begin() + _idx);

    if (it->second.empty())
        data_map_.erase(it);

    return removed;
}

bool XDataImpl::DataReset(uint64_t _data_uid)
{
    //std::unique_lock lck(map_rw_);

    return data_map_.erase(_data_uid) > 0;
}

} // namespace impl
} // namespace xsdk