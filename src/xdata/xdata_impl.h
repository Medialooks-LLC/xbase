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

    XDataImpl(std::map<uint64_t, data_entry>&& _data_map);

public:
    XDataImpl() : uid_(xbase::NextUid()) {}

public:
    //-------------------------------------------------------------------------------
    virtual IData::UPtr Clone(const std::set<uint64_t>& _cloned_types, CloneSetType _set_type) const override;
    virtual size_t      DataSet(uint64_t _data_uid, std::any&& _face, std::any&& _holder, size_t _idx) override;
    virtual size_t      DataCount(uint64_t _data_uid) const override;
    virtual std::pair<std::any, std::any> DataGet(uint64_t _data_uid, size_t _idx = 0) const override;
    virtual std::pair<std::any, std::any> DataRemove(uint64_t _data_uid, size_t _idx = 0) override;
    virtual bool                          DataReset(uint64_t _data_uid) override;

private:
    const uint64_t                 uid_;
    std::map<uint64_t, data_entry> data_map_;
};

} // namespace xsdk::impl