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
