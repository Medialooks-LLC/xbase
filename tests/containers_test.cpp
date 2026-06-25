#include "xbase/containers.hpp"

#include <gtest/gtest.h>

#include <map>
#include <shared_mutex>

namespace xsdk::xbase::containers {

TEST(ContainersMapSharedGet, DoesNotCreateExistingValue)
{
    std::shared_mutex         rw_mtx;
    std::map<std::string, int> values = {{"key", 7}};
    bool                      create_called = false;

    auto [value, inserted] = MapSharedGet<std::map<std::string, int>>(rw_mtx, values, "key", [&]() {
        create_called = true;
        return 42;
    });

    EXPECT_EQ(value, 7);
    EXPECT_FALSE(inserted);
    EXPECT_FALSE(create_called);
}

TEST(ContainersMapSharedGet, CreatesMissingValue)
{
    std::shared_mutex          rw_mtx;
    std::map<std::string, int> values;

    auto [value, inserted] = MapSharedGet<std::map<std::string, int>>(rw_mtx, values, "key", []() { return 42; });

    EXPECT_EQ(value, 42);
    EXPECT_TRUE(inserted);
    EXPECT_EQ(values["key"], 42);
}

} // namespace xsdk::xbase::containers
