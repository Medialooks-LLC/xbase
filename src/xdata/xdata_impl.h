#pragma once

#include "xbase/xdata.h"

#include <cassert>
#include <memory>
#include <string>

#include <functional>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace xsdk::impl {

class XDataImpl: public IData {

    using data_entry = std::vector<std::pair<std::any, std::any>>;

    explicit XDataImpl(std::map<uint64_t, data_entry>&& _data_map);

public:
    XDataImpl() : uid_(xbase::NextUid()) {}

public:
    //-------------------------------------------------------------------------------
    virtual IData::UPtr Clone(const std::set<uint64_t>& _cloned_types, CloneSetType _set_type) const override;
    virtual size_t      CopyTo(IData*                    _dest,
                               const bool                _overwrite,
                               const std::set<uint64_t>& _copy_types = {},
                               const CloneSetType        _set_type   = CloneSetType::Exclude) const override;

    virtual size_t DataSet(const uint64_t _data_uid, std::any&& _face, std::any&& _holder, const size_t _idx) override;
    virtual size_t DataCount(const uint64_t _data_uid) const override;
    virtual std::pair<std::any, std::any> DataGet(const uint64_t _data_uid, const size_t _idx = 0) const override;
    virtual std::pair<std::any, std::any> DataRemove(const uint64_t _data_uid, const size_t _idx = 0) override;
    virtual bool                          DataReset(const uint64_t _data_uid) override;
    virtual size_t                        TypesCount() const override { return data_map_.size(); };

private:
    const uint64_t                 uid_;
    std::map<uint64_t, data_entry> data_map_;
};

} // namespace xsdk::impl