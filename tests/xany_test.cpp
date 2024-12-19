#include "xbase.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <thread>

using namespace xsdk;

// NOLINTBEGIN(*)

class TestClass {
    const std::string* data_ = nullptr;

public:
    TestClass(TestClass&& _move) { std::swap(data_, _move.data_); };
    explicit TestClass(std::string&& _str) : data_(new std::string(std::move(_str))) {};
    ~TestClass()
    {
        if (data_)
            delete data_;
    }
    std::string Str() const { return *data_; }
};
TEST(xany_tests, any_set)
{
    std::string str_test = "test string";
    auto        copy     = str_test;
    xbase::XAny any_str(std::move(copy));
    xbase::XAny any_str2((std::string)str_test);

    const auto* p_null = any_str.AnyCast<std::string_view>();
    EXPECT_FALSE(p_null);

    const auto* p_str = any_str.AnyCast<std::string>();
    ASSERT_TRUE(p_str);
    EXPECT_EQ(*p_str, str_test);

    const auto* p_str2 = any_str2.AnyCast<std::string>();
    ASSERT_TRUE(p_str2);
    EXPECT_EQ(*p_str2, str_test);

    xbase::XAny any_test(TestClass((std::string)str_test));

    EXPECT_FALSE(any_test.AnyCast<std::string>());

    const auto* p_test = any_test.AnyCast<TestClass>();
    ASSERT_TRUE(p_test);
    EXPECT_EQ(p_test->Str(), str_test);
}

// NOLINTEND(*)