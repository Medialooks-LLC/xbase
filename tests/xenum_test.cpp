#include "xbase.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <thread>

using namespace xsdk;

namespace xtest {

XENUM_CLASS(SockStatusTest,
            NONEXIST   = 0,
            INIT       = 0x1,
            OPENED     = 0x2,
            LISTENING  = 0x4,
            CONNECTING = 0x8,
            CONNECTED  = 0x10,
            BROKEN     = 0x20,
            CLOSING    = 0x40,
            CLOSED     = 0x80,

            RECONNECT = 0x100,

            kNotValid = 0x1000,
            kChecking = 0x2000,

            RECONNECT_INIT       = RECONNECT | INIT,
            RECONNECT_OPENED     = RECONNECT | OPENED,
            RECONNECT_LISTENING  = RECONNECT | LISTENING,
            RECONNECT_CONNECTING = RECONNECT | CONNECTING,
            RECONNECT_CONNECTED  = RECONNECT | CONNECTED,
            RECONNECT_BROKEN     = RECONNECT | BROKEN,
            RECONNECT_CLOSING    = RECONNECT | CLOSING,
            RECONNECT_CLOSED     = RECONNECT | CLOSED,
            RECONNECT_NONEXIST   = RECONNECT | NONEXIST)
}
TEST(xenum_tests, enum_basic)
{
    using namespace xtest;

    SockStatusTest test = SockStatusTest::CLOSING;
    EXPECT_EQ(xenum::ToString(test), "CLOSING");

    test = test | SockStatusTest::RECONNECT;
    EXPECT_EQ(xenum::ToString(test), "RECONNECT_CLOSING");

    auto check = xenum::FromString("RECONNECT_LISTENING", SockStatusTest::BROKEN);
    EXPECT_TRUE(check == SockStatusTest::RECONNECT_LISTENING);
    EXPECT_TRUE((bool)(check & SockStatusTest::LISTENING));
    EXPECT_TRUE((bool)(check & SockStatusTest::RECONNECT));

    SockStatusTest rec_open = SockStatusTest::OPENED | SockStatusTest::RECONNECT;
    EXPECT_EQ(rec_open, SockStatusTest::RECONNECT_OPENED);
    EXPECT_EQ(xenum::ToString(rec_open), "RECONNECT_OPENED");

    SockStatusTest wrong      = SockStatusTest::OPENED | SockStatusTest::CLOSING;
    auto           wrong_str0 = xenum::ToString(wrong, "WRONG_COMBO");
    std::cout << "CANT CONVERT:" << wrong_str0 << std::endl;
    EXPECT_EQ(wrong_str0, "WRONG_COMBO");

    auto wrong_enum = xenum::FromString<SockStatusTest>("RECONNECT_XXX", SockStatusTest::CLOSED);
    EXPECT_EQ(wrong_enum, SockStatusTest::CLOSED);

    auto wrong_str = xenum::ToString(SockStatusTest(99));
    std::cout << "CANT CONVERT:" << wrong_str << std::endl;
    EXPECT_EQ(wrong_str, "xtest::SockStatusTest(99)");
    wrong_str = xenum::ToString(SockStatusTest(99), "WRONG");
    EXPECT_EQ(wrong_str, "WRONG");
}

TEST(xenum_tests, enum_complex)
{
    using namespace xtest;

    auto test = xenum::FromString<SockStatusTest>("OPENED|CLOSING|RECONNECT");
    ASSERT_TRUE(test.has_value());
    EXPECT_EQ(test.value(), SockStatusTest::OPENED | SockStatusTest::CLOSING | SockStatusTest::RECONNECT);

    test = xenum::FromString<SockStatusTest>("|OPENED||CLOSING|RECONNECT");
    ASSERT_TRUE(test.has_value());
    EXPECT_EQ(test.value(), SockStatusTest::OPENED | SockStatusTest::CLOSING | SockStatusTest::RECONNECT);

    test = xenum::FromString<SockStatusTest>("INIT|OPENED|CLOSING RECONNECT");
    ASSERT_TRUE(test.has_value());
    EXPECT_EQ(test.value(), SockStatusTest::OPENED | SockStatusTest::INIT);

    test = xenum::FromString<SockStatusTest>("OPENED|CLOSING_XXX|RECONNECT|");
    ASSERT_TRUE(test.has_value());
    EXPECT_EQ(test.value(), SockStatusTest::OPENED | SockStatusTest::RECONNECT);

    test = xenum::FromString<SockStatusTest>("ZZZ||CLOSING_XXX");
    EXPECT_FALSE(test.has_value());

    test = xenum::FromString<SockStatusTest>({});
    EXPECT_FALSE(test.has_value());
}

TEST(xenum_tests, enum_complex_prefix)
{
    using namespace xtest;

    auto test = xenum::FromString<SockStatusTest>("kNotValid");
    ASSERT_TRUE(test.has_value());
    EXPECT_EQ(test.value(), SockStatusTest::kNotValid);

    test = xenum::FromString<SockStatusTest>("NotValid|kChecking|OPENED");
    ASSERT_TRUE(test.has_value());
    EXPECT_EQ(test.value(), SockStatusTest::kNotValid | SockStatusTest::kChecking | SockStatusTest::OPENED);

    test = xenum::FromString<SockStatusTest>("NotValid|Checking|kOPENED");
    ASSERT_TRUE(test.has_value());
    EXPECT_EQ(test.value(), SockStatusTest::kNotValid | SockStatusTest::kChecking);

}
