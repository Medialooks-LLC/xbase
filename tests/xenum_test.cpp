#include "xbase.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <thread>

namespace xsdk::xbase::enums {

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

XENUM_CLASS64(LargeSockStatusTest,
              NONE     = 0,
              INIT     = 1LL << 0,
              OPENED   = 1LL << 1,
              HIGH     = 1LL << 40,
              HIGHER   = 1LL << 41,
              kBigFlag = 1LL << 42,

              HIGH_OPENED = HIGH | OPENED,
              ALL_HIGH    = HIGH | HIGHER | kBigFlag)

struct NestedEnumClassTestHolder {
    XENUM_NESTED(State,
                 kStopped    = 0,
                 kStarting   = 0x1,
                 kRunning    = 0x2,
                 kPaused     = 0x4,
                 kBroken     = 0x8,
                 kRecovering = 0x10,

                 kActive            = kStarting | kRunning,
                 kRecoveringRunning = kRecovering | kRunning)
};

struct NestedEnumClass64TestHolder {
    XENUM_NESTED64(State,
                   kNone     = 0,
                   kStarting = 1LL << 0,
                   kRunning  = 1LL << 1,
                   kHigh     = 1LL << 40,
                   kHigher   = 1LL << 41,

                   kActive     = kStarting | kRunning,
                   kHighActive = kHigh | kRunning,
                   kAllHigh    = kHigh | kHigher)
};

TEST(xenum_tests, enum_basic)
{

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
    EXPECT_EQ(wrong_str, "xsdk::xbase::enums::SockStatusTest(99)");
    wrong_str = xenum::ToString(SockStatusTest(99), "WRONG");
    EXPECT_EQ(wrong_str, "WRONG");
}

TEST(xenum_tests, enum_complex)
{

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

TEST(xenum_tests, enum_nested_class_basic)
{
    using State = NestedEnumClassTestHolder::State;

    State test = State::kRunning;
    EXPECT_EQ(xenum::ToString(test), "kRunning");

    test = test | State::kRecovering;
    EXPECT_EQ(test, State::kRecoveringRunning);
    EXPECT_EQ(xenum::ToString(test), "kRecoveringRunning");

    auto check = xenum::FromString("kPaused", State::kBroken);
    EXPECT_EQ(check, State::kPaused);

    auto wrong_enum = xenum::FromString<State>("kMissing", State::kStopped);
    EXPECT_EQ(wrong_enum, State::kStopped);

    auto wrong_str = xenum::ToString(State(99), "WRONG_NESTED");
    EXPECT_EQ(wrong_str, "WRONG_NESTED");
}

TEST(xenum_tests, enum_nested_class_complex)
{
    using State = NestedEnumClassTestHolder::State;

    auto test = xenum::FromString<State>("kStarting|kRunning");
    ASSERT_TRUE(test.has_value());
    EXPECT_EQ(test.value(), State::kActive);

    test = xenum::FromString<State>("Starting|Running|kRecovering");
    ASSERT_TRUE(test.has_value());
    EXPECT_EQ(test.value(), State::kActive | State::kRecovering);

    test = xenum::FromString<State>("kMissing|kPaused");
    ASSERT_TRUE(test.has_value());
    EXPECT_EQ(test.value(), State::kPaused);

    test = xenum::FromString<State>("kMissing|Broken_XXX");
    EXPECT_FALSE(test.has_value());
}

TEST(xenum_tests, enum64_basic)
{
    LargeSockStatusTest test = LargeSockStatusTest::HIGH;
    EXPECT_EQ(xenum::ToString(test), "HIGH");

    test |= LargeSockStatusTest::OPENED;
    EXPECT_EQ(test, LargeSockStatusTest::HIGH_OPENED);
    EXPECT_EQ(xenum::ToString(test), "HIGH_OPENED");

    test &= LargeSockStatusTest::HIGH;
    EXPECT_EQ(test, LargeSockStatusTest::HIGH);
    EXPECT_TRUE(xenum::HasFlag(test, LargeSockStatusTest::HIGH));
    EXPECT_FALSE(xenum::HasFlag(test, LargeSockStatusTest::HIGHER));

    EXPECT_TRUE((bool)(LargeSockStatusTest::ALL_HIGH & LargeSockStatusTest::kBigFlag));

    const auto wrong_str = xenum::ToString(LargeSockStatusTest(1LL << 45), "WRONG_64");
    EXPECT_EQ(wrong_str, "WRONG_64");
}

TEST(xenum_tests, enum64_from_string)
{
    auto test = xenum::FromStringOne<LargeSockStatusTest>("HIGH");
    ASSERT_TRUE(test.has_value());
    EXPECT_EQ(test.value(), LargeSockStatusTest::HIGH);

    test = xenum::FromStringOne<LargeSockStatusTest>("BigFlag");
    ASSERT_TRUE(test.has_value());
    EXPECT_EQ(test.value(), LargeSockStatusTest::kBigFlag);

    auto combined = xenum::FromString<LargeSockStatusTest>("HIGH|HIGHER|BigFlag");
    ASSERT_TRUE(combined.has_value());
    EXPECT_EQ(combined.value(), LargeSockStatusTest::ALL_HIGH);

    combined = xenum::FromString<LargeSockStatusTest>("HIGH|MISSING_64");
    ASSERT_TRUE(combined.has_value());
    EXPECT_EQ(combined.value(), LargeSockStatusTest::HIGH);

    combined = xenum::FromString<LargeSockStatusTest>("MISSING_64|ANOTHER_MISSING_64");
    EXPECT_FALSE(combined.has_value());

    const auto fallback = xenum::FromString<LargeSockStatusTest>("MISSING_64", LargeSockStatusTest::NONE);
    EXPECT_EQ(fallback, LargeSockStatusTest::NONE);
}

TEST(xenum_tests, enum64_nested_class_basic)
{
    using State = NestedEnumClass64TestHolder::State;

    State test = State::kHigh;
    EXPECT_EQ(xenum::ToString(test), "kHigh");

    test |= State::kRunning;
    EXPECT_EQ(test, State::kHighActive);
    EXPECT_EQ(xenum::ToString(test), "kHighActive");

    test &= State::kHigh;
    EXPECT_EQ(test, State::kHigh);

    auto check = xenum::FromString("High", State::kNone);
    EXPECT_EQ(check, State::kHigh);

    auto wrong_enum = xenum::FromString<State>("kMissing64", State::kNone);
    EXPECT_EQ(wrong_enum, State::kNone);

    auto wrong_str = xenum::ToString(State(1LL << 45), "WRONG_NESTED_64");
    EXPECT_EQ(wrong_str, "WRONG_NESTED_64");
}

TEST(xenum_tests, enum64_nested_class_complex)
{
    using State = NestedEnumClass64TestHolder::State;

    auto test = xenum::FromString<State>("kHigh|kHigher");
    ASSERT_TRUE(test.has_value());
    EXPECT_EQ(test.value(), State::kAllHigh);

    test = xenum::FromString<State>("High|Running");
    ASSERT_TRUE(test.has_value());
    EXPECT_EQ(test.value(), State::kHighActive);

    test = xenum::FromString<State>("kMissing64|kHigher");
    ASSERT_TRUE(test.has_value());
    EXPECT_EQ(test.value(), State::kHigher);

    test = xenum::FromString<State>("kMissing64|MissingAgain64");
    EXPECT_FALSE(test.has_value());
}

TEST(xenum_tests, has_flag_zero_semantics)
{
    EXPECT_TRUE(xenum::HasFlag(LargeSockStatusTest::NONE, LargeSockStatusTest::NONE));
    EXPECT_FALSE(xenum::HasFlag(LargeSockStatusTest::HIGH, LargeSockStatusTest::NONE));

    using NestedState = NestedEnumClass64TestHolder::State;
    EXPECT_TRUE(xenum::HasFlag(NestedState::kNone, NestedState::kNone));
    EXPECT_FALSE(xenum::HasFlag(NestedState::kHigh, NestedState::kNone));
}

} // namespace xsdk::xbase::enums
