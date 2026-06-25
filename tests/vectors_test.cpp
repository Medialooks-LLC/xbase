#include "xbase.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace xsdk::xvector {

TEST(VectorsCompare, UsesMemcmpCompatibleOrderingForNulls)
{
    const std::vector<uint8_t> right = {1};

    EXPECT_LT(Compare<uint8_t>(nullptr, &right), 0);
    EXPECT_GT(Compare<uint8_t>(&right, nullptr), 0);
    EXPECT_EQ(Compare<uint8_t>(nullptr, nullptr), 0);
}

TEST(VectorsCompare, UsesMemcmpCompatibleOrderingForSizes)
{
    const std::vector<uint8_t> left  = {1};
    const std::vector<uint8_t> right = {1, 2};

    EXPECT_LT(Compare(&left, &right), 0);
    EXPECT_GT(Compare(&right, &left), 0);
}

TEST(VectorsCompare, UsesMemcmpCompatibleOrderingForData)
{
    const std::vector<uint8_t> left  = {1, 2};
    const std::vector<uint8_t> right = {1, 3};

    EXPECT_LT(Compare(&left, &right), 0);
    EXPECT_GT(Compare(&right, &left), 0);
}

TEST(VectorsCompare, ComparesBlobViews)
{
    const std::vector<uint8_t> left  = {1};
    const std::vector<uint8_t> right = {1, 2};

    EXPECT_LT(Compare<uint8_t>({left.size(), left.data()}, {right.size(), right.data()}), 0);
    EXPECT_GT(Compare<uint8_t>({right.size(), right.data()}, {left.size(), left.data()}), 0);
}

} // namespace xsdk::xvector
