#include "xbase.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace xsdk;

class MyTest {
    std::string val_;

public:
    using UPtr = std::unique_ptr<MyTest>;
    using SPtr = std::shared_ptr<MyTest>;

    MyTest(std::string&& _val) : val_(std::move(_val)) {}
    ~MyTest() { val_.clear(); }

    const std::string& Val() const { return val_; }
};

class MyTestU {
    std::unique_ptr<std::string> val_p_;

public:
    MyTestU(std::string&& _val) : val_p_(std::make_unique<std::string>(std::move(_val))) {}
    MyTestU(const MyTestU& _copy) : val_p_(std::make_unique<std::string>(_copy.Val())) {}

    ~MyTestU() { val_p_.reset(); }

    const std::string& Val() const { return *val_p_; }
};
TEST(xresult_tests, check_result_cref_0)
{
    xbase::XResult<std::string> empty_res;

    auto error_res = empty_res.Result("test_string");
    std::cout << error_res;
    EXPECT_EQ(error_res, "test_string");

    EXPECT_EQ(empty_res.Result("test_string2"), "test_string2");
}

TEST(xresult_tests, check_result_cref_1)
{
    xbase::XResult<MyTest> empty_res;

    auto error_res = empty_res.Result(std::string("test_string"));
    std::cout << error_res.Val();
    EXPECT_EQ(error_res.Val(), "test_string");

    EXPECT_EQ(empty_res.Result(std::string("test_string2")).Val(), "test_string2");
}

TEST(xresult_tests, check_result_cref_2)
{
    xbase::XResult<MyTestU> empty_res;

    auto error_res = empty_res.Result(std::string("test_string"));
    std::cout << error_res.Val();
    EXPECT_EQ(error_res.Val(), "test_string");

    EXPECT_EQ(empty_res.Result(std::string("test_string2")).Val(), "test_string2");
}

TEST(xresult_tests, check_result_cref_3)
{
    xbase::XResult<MyTest::UPtr> empty_res;

    static MyTest::UPtr for_error = std::make_unique<MyTest>("test_string");

    const auto& error_res = empty_res.Result(for_error);
    std::cout << error_res->Val();
    EXPECT_EQ(error_res->Val(), "test_string");
}

TEST(xresult_tests, result_operator)
{
    xbase::XResult<MyTest> res = MyTest(std::string("result1"));
    ASSERT_TRUE(res.HasResult());
    EXPECT_EQ(res->Val(), "result1");

    xbase::XResult<MyTestU> res2 = MyTestU(std::string("result2"));
    ASSERT_TRUE(res2.HasResult());
    EXPECT_EQ(res2->Val(), "result2");
}

TEST(xresult_tests, operator_exception)
{
    xbase::XResult<MyTest> res;
    ASSERT_TRUE(res.Empty());
    ASSERT_FALSE(res.HasError());
    ASSERT_FALSE(res.HasResult());
    // TODO:
    /*
#ifdef NDEBUG
    try {
        auto check = res->Val();
    }
    catch (const std::exception& ex) {
        std::cout << ex.what();
    }
#endif
*/
}

TEST(xresult_tests, operator_exception_2)
{
    xbase::XResult<MyTest::SPtr> res = std::make_error_code(std::errc::address_in_use);
    ASSERT_FALSE(res.Empty());
    ASSERT_TRUE(res.HasError());
    ASSERT_FALSE(res.HasResult());
    // TODO:
    /*
#ifdef NDEBUG
    try {
        auto check = res->Val();
    }
    catch (const std::exception& ex) {
        std::cout << ex.what();
    }
#endif
*/
}

TEST(xresult_tests, shared_ptr)
{
    auto shared_ptr = std::make_shared<MyTest>(std::string("result1"));

    xbase::XResult<MyTest::SPtr> xr = shared_ptr;
    ASSERT_TRUE(xr.HasResult());
    EXPECT_EQ(xr->Val(), "result1");
    EXPECT_EQ(xr.GetPtr(), shared_ptr.get());
}

TEST(xresult_tests, unique_ptr)
{
    auto        unique_ptr = std::make_unique<MyTest>(std::string("result1"));
    const auto* raw_ptr    = unique_ptr.get();

    xbase::XResult<MyTest::UPtr> xr = std::move(unique_ptr);
    ASSERT_TRUE(xr.HasResult());
    EXPECT_EQ(xr->Val(), "result1");
    EXPECT_EQ(xr.GetPtr(), raw_ptr);
}