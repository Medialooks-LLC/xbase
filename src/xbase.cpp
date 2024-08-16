#include "xbase/xuid.h"

#include <atomic>

namespace xsdk::xbase {

uint64_t NextUid()
{
    static std::atomic<uint64_t> counter = {1};
    return counter.fetch_add(1);
}

} // namespace xsdk::xbase