/**
 * @file numbers_test.cpp
 * @brief Unit tests for reviewed numeric helpers.
 */

#include "xbase/numbers.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>

namespace xsdk::xbase::numbers {

TEST(OptionalConvertIntegralTest, ReturnsNulloptWhenSourceIsEmpty)
{
    EXPECT_EQ(OptionalConvert<int32_t>(std::optional<int64_t> {}, false), std::nullopt);
}

TEST(OptionalConvertIntegralTest, ReturnsSameValueForSameType)
{
    EXPECT_EQ(OptionalConvert<int32_t>(std::optional<int32_t> {123}, false), std::optional<int32_t> {123});
}

TEST(OptionalConvertIntegralTest, ConvertsSignedToUnsignedPositiveValue)
{
    EXPECT_EQ(OptionalConvert<uint32_t>(std::optional<int32_t> {42}, false), std::optional<uint32_t> {42u});
}

TEST(OptionalConvertIntegralTest, ReturnsNulloptForNegativeSignedToUnsignedWithoutClamp)
{
    EXPECT_EQ(OptionalConvert<uint32_t>(std::optional<int32_t> {-1}, false), std::nullopt);
}

TEST(OptionalConvertIntegralTest, ClampsNegativeSignedToUnsignedToZero)
{
    EXPECT_EQ(OptionalConvert<uint32_t>(std::optional<int32_t> {-1}, true), std::optional<uint32_t> {0u});
}

TEST(OptionalConvertIntegralTest, ReturnsNulloptWhenSignedValueIsAboveTargetMaxWithoutClamp)
{
    const int32_t kAboveMax = static_cast<int32_t>(std::numeric_limits<int16_t>::max()) + 1;
    EXPECT_EQ(OptionalConvert<int16_t>(std::optional<int32_t> {kAboveMax}, false), std::nullopt);
}

TEST(OptionalConvertIntegralTest, ClampsWhenSignedValueIsAboveTargetMax)
{
    const int32_t kAboveMax = static_cast<int32_t>(std::numeric_limits<int16_t>::max()) + 1;
    EXPECT_EQ(OptionalConvert<int16_t>(std::optional<int32_t> {kAboveMax}, true),
              std::optional<int16_t> {std::numeric_limits<int16_t>::max()});
}

TEST(OptionalConvertIntegralTest, ConvertsUnsignedToSignedSameSizeWhenValueFits)
{
    EXPECT_EQ(OptionalConvert<int32_t>(std::optional<uint32_t> {42u}, false), std::optional<int32_t> {42});
}

TEST(OptionalConvertIntegralTest, ConvertsUnsignedToSignedAtExactMaxBoundary)
{
    const uint32_t kMaxInt32 = static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
    EXPECT_EQ(OptionalConvert<int32_t>(std::optional<uint32_t> {kMaxInt32}, false),
              std::optional<int32_t> {std::numeric_limits<int32_t>::max()});
}

TEST(OptionalConvertIntegralTest, ReturnsNulloptWhenUnsignedToSignedIsAboveMaxWithoutClamp)
{
    const uint32_t kOverflow = static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) + 1u;
    EXPECT_EQ(OptionalConvert<int32_t>(std::optional<uint32_t> {kOverflow}, false), std::nullopt);
}

TEST(OptionalConvertIntegralTest, ClampsUnsignedToSignedToMax)
{
    const uint32_t kOverflow = static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) + 1u;
    EXPECT_EQ(OptionalConvert<int32_t>(std::optional<uint32_t> {kOverflow}, true),
              std::optional<int32_t> {std::numeric_limits<int32_t>::max()});
}

TEST(OptionalConvertFloatingTest, RoundsPositiveToNearestSignedInteger)
{
    EXPECT_EQ(OptionalConvert<int32_t>(std::optional<double> {1.4}, false), std::optional<int32_t> {1});
    EXPECT_EQ(OptionalConvert<int32_t>(std::optional<double> {1.5}, false), std::optional<int32_t> {2});
    EXPECT_EQ(OptionalConvert<int32_t>(std::optional<double> {1.6}, false), std::optional<int32_t> {2});
}

TEST(OptionalConvertFloatingTest, RoundsNegativeToNearestSignedInteger)
{
    EXPECT_EQ(OptionalConvert<int32_t>(std::optional<double> {-1.4}, false), std::optional<int32_t> {-1});
    EXPECT_EQ(OptionalConvert<int32_t>(std::optional<double> {-1.5}, false), std::optional<int32_t> {-2});
    EXPECT_EQ(OptionalConvert<int32_t>(std::optional<double> {-1.6}, false), std::optional<int32_t> {-2});
}

TEST(OptionalConvertFloatingTest, RoundsPositiveToNearestUnsignedInteger)
{
    EXPECT_EQ(OptionalConvert<uint32_t>(std::optional<double> {1.4}, false), std::optional<uint32_t> {1u});
    EXPECT_EQ(OptionalConvert<uint32_t>(std::optional<double> {1.5}, false), std::optional<uint32_t> {2u});
}

TEST(OptionalConvertFloatingTest, SmallNegativeRoundsToZeroForUnsigned)
{
    EXPECT_EQ(OptionalConvert<uint32_t>(std::optional<double> {-0.4}, false), std::optional<uint32_t> {0u});
    EXPECT_EQ(OptionalConvert<uint32_t>(std::optional<double> {-0.49}, false), std::optional<uint32_t> {0u});
}

TEST(OptionalConvertFloatingTest, MinusHalfOrLessIsRejectedForUnsignedWithoutClamp)
{
    EXPECT_EQ(OptionalConvert<uint32_t>(std::optional<double> {-0.5}, false), std::nullopt);
    EXPECT_EQ(OptionalConvert<uint32_t>(std::optional<double> {-1.0}, false), std::nullopt);
}

TEST(OptionalConvertFloatingTest, MinusHalfOrLessIsClampedToZeroForUnsignedWithClamp)
{
    EXPECT_EQ(OptionalConvert<uint32_t>(std::optional<double> {-0.5}, true), std::optional<uint32_t> {0u});
    EXPECT_EQ(OptionalConvert<uint32_t>(std::optional<double> {-100.0}, true), std::optional<uint32_t> {0u});
}

TEST(OptionalConvertFloatingTest, ClampsFloatingToIntegralMax)
{
    EXPECT_EQ(OptionalConvert<int32_t>(std::optional<double> {1e100}, true),
              std::optional<int32_t> {std::numeric_limits<int32_t>::max()});
}

TEST(OptionalConvertFloatingTest, ClampsFloatingToIntegralMin)
{
    EXPECT_EQ(OptionalConvert<int32_t>(std::optional<double> {-1e100}, true),
              std::optional<int32_t> {std::numeric_limits<int32_t>::lowest()});
}

TEST(OptionalConvertFloatingTest, ConvertsIntegralToFloating)
{
    const auto result = OptionalConvert<double>(std::optional<int32_t> {42}, false);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result.value(), 42.0);
}

TEST(OptionalConvertFloatingTest, ConvertsFloatingToFloating)
{
    const auto result = OptionalConvert<float>(std::optional<double> {3.25}, false);
    ASSERT_TRUE(result.has_value());
    EXPECT_FLOAT_EQ(result.value(), 3.25F);
}

TEST(OptionalConvertFloatingTest, ReturnsNulloptForNaN)
{
    const double kNaN = std::numeric_limits<double>::quiet_NaN();
    EXPECT_EQ(OptionalConvert<float>(std::optional<double> {kNaN}, false), std::nullopt);
}

TEST(OptionalConvertFloatingTest, ClampsInfinityToFloatingMax)
{
    const double kInf = std::numeric_limits<double>::infinity();
    EXPECT_EQ(OptionalConvert<float>(std::optional<double> {kInf}, true),
              std::optional<float> {std::numeric_limits<float>::max()});
}

TEST(OptionalAddIntegralTest, ReturnsSourceWhenOptionalIsEmpty)
{
    EXPECT_EQ(OptionalAdd<int32_t>(std::optional<int32_t> {}, 5, false), std::nullopt);
}

TEST(OptionalAddIntegralTest, ReturnsSourceWhenAddValueIsZero)
{
    EXPECT_EQ(OptionalAdd<int32_t>(std::optional<int32_t> {7}, 0, false), std::optional<int32_t> {7});
}

TEST(OptionalAddIntegralTest, AddsSignedValueWithoutOverflow)
{
    EXPECT_EQ(OptionalAdd<int32_t>(std::optional<int32_t> {10}, 5, false), std::optional<int32_t> {15});
}

TEST(OptionalAddIntegralTest, ReturnsNulloptOnSignedPositiveOverflowWithoutClamp)
{
    EXPECT_EQ(OptionalAdd<int32_t>(std::optional<int32_t> {std::numeric_limits<int32_t>::max()}, 1, false),
              std::nullopt);
}

TEST(OptionalAddIntegralTest, ClampsOnSignedPositiveOverflow)
{
    EXPECT_EQ(OptionalAdd<int32_t>(std::optional<int32_t> {std::numeric_limits<int32_t>::max()}, 1, true),
              std::optional<int32_t> {std::numeric_limits<int32_t>::max()});
}

TEST(OptionalAddIntegralTest, ReturnsNulloptOnSignedNegativeOverflowWithoutClamp)
{
    EXPECT_EQ(OptionalAdd<int32_t>(std::optional<int32_t> {std::numeric_limits<int32_t>::lowest()}, -1, false),
              std::nullopt);
}

TEST(OptionalAddIntegralTest, ClampsOnSignedNegativeOverflow)
{
    EXPECT_EQ(OptionalAdd<int32_t>(std::optional<int32_t> {std::numeric_limits<int32_t>::lowest()}, -1, true),
              std::optional<int32_t> {std::numeric_limits<int32_t>::lowest()});
}

TEST(OptionalAddIntegralTest, AddsUnsignedValueWithoutOverflow)
{
    EXPECT_EQ(OptionalAdd<uint32_t>(std::optional<uint32_t> {10u}, 5u, false), std::optional<uint32_t> {15u});
}

TEST(OptionalAddIntegralTest, ReturnsNulloptOnUnsignedOverflowWithoutClamp)
{
    EXPECT_EQ(OptionalAdd<uint32_t>(std::optional<uint32_t> {std::numeric_limits<uint32_t>::max()}, 1u, false),
              std::nullopt);
}

TEST(OptionalAddIntegralTest, ClampsOnUnsignedOverflow)
{
    EXPECT_EQ(OptionalAdd<uint32_t>(std::optional<uint32_t> {std::numeric_limits<uint32_t>::max()}, 1u, true),
              std::optional<uint32_t> {std::numeric_limits<uint32_t>::max()});
}

TEST(OptionalAddFloatingTest, AddsDoubleValues)
{
    const auto result = OptionalAdd<double>(std::optional<double> {1.5}, 2.25, false);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result.value(), 3.75);
}

TEST(OptionalAddFloatingTest, ReturnsSourceWhenFloatingAddValueIsZero)
{
    const auto result = OptionalAdd<double>(std::optional<double> {7.5}, 0.0, false);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result.value(), 7.5);
}

TEST(OptionalAddFloatingTest, ReturnsNulloptForNaNInput)
{
    const double kNaN = std::numeric_limits<double>::quiet_NaN();
    EXPECT_EQ(OptionalAdd<double>(std::optional<double> {kNaN}, 1.0, false), std::nullopt);
}

TEST(OptionalAddFloatingTest, ReturnsNulloptForNaNAddend)
{
    const double kNaN = std::numeric_limits<double>::quiet_NaN();
    EXPECT_EQ(OptionalAdd<double>(std::optional<double> {1.0}, kNaN, false), std::nullopt);
}

TEST(OptionalAddFloatingTest, ReturnsNulloptOnPositiveOverflowWithoutClamp)
{
    EXPECT_EQ(OptionalAdd<double>(std::optional<double> {std::numeric_limits<double>::max()},
                                  std::numeric_limits<double>::max(),
                                  false),
              std::nullopt);
}

TEST(OptionalAddFloatingTest, ClampsOnPositiveOverflow)
{
    EXPECT_EQ(OptionalAdd<double>(std::optional<double> {std::numeric_limits<double>::max()},
                                  std::numeric_limits<double>::max(),
                                  true),
              std::optional<double> {std::numeric_limits<double>::max()});
}

TEST(OptionalAddFloatingTest, ClampsOnNegativeOverflow)
{
    EXPECT_EQ(OptionalAdd<double>(std::optional<double> {std::numeric_limits<double>::lowest()},
                                  std::numeric_limits<double>::lowest(),
                                  true),
              std::optional<double> {std::numeric_limits<double>::lowest()});
}

TEST(ClampTest, ClampsSignedToSignedBelowMin)
{
    EXPECT_EQ(Clamp<int8_t>(static_cast<int32_t>(-200)), std::numeric_limits<int8_t>::lowest());
}

TEST(ClampTest, ClampsSignedToSignedAboveMax)
{
    EXPECT_EQ(Clamp<int8_t>(static_cast<int32_t>(200)), std::numeric_limits<int8_t>::max());
}

TEST(ClampTest, ClampsSignedNegativeToUnsignedZero)
{
    EXPECT_EQ(Clamp<uint8_t>(static_cast<int32_t>(-1)), static_cast<uint8_t>(0));
}

TEST(ClampTest, ConvertsSignedPositiveToUnsignedWhenFits)
{
    EXPECT_EQ(Clamp<uint8_t>(static_cast<int32_t>(42)), static_cast<uint8_t>(42));
}

TEST(ClampTest, ClampsSignedPositiveToUnsignedMax)
{
    EXPECT_EQ(Clamp<uint8_t>(static_cast<int32_t>(300)), std::numeric_limits<uint8_t>::max());
}

TEST(ClampTest, ConvertsUnsignedToSignedWhenFits) { EXPECT_EQ(Clamp<int32_t>(static_cast<uint32_t>(42u)), 42); }

TEST(ClampTest, ClampsUnsignedToSignedMax)
{
    const uint32_t kOverflow = static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) + 1u;
    EXPECT_EQ(Clamp<int32_t>(kOverflow), std::numeric_limits<int32_t>::max());
}

TEST(ClampTest, ClampsUnsignedToUnsignedMax)
{
    EXPECT_EQ(Clamp<uint8_t>(static_cast<uint32_t>(300u)), std::numeric_limits<uint8_t>::max());
}

TEST(ClampFloatingTest, RoundsFloatingToNearestSignedInteger)
{
    EXPECT_EQ(Clamp<int32_t>(1.4), 1);
    EXPECT_EQ(Clamp<int32_t>(1.5), 2);
    EXPECT_EQ(Clamp<int32_t>(-1.5), -2);
}

TEST(ClampFloatingTest, RoundsSmallNegativeToZeroForUnsigned) { EXPECT_EQ(Clamp<uint32_t>(-0.4), 0u); }

TEST(ClampFloatingTest, ClampsFloatingToIntegralMax)
{
    EXPECT_EQ(Clamp<int32_t>(1e100), std::numeric_limits<int32_t>::max());
}

TEST(ClampFloatingTest, ClampsFloatingToIntegralMin)
{
    EXPECT_EQ(Clamp<int32_t>(-1e100), std::numeric_limits<int32_t>::lowest());
}

TEST(ClampFloatingTest, ReturnsZeroForNaNToIntegral)
{
    const double kNaN = std::numeric_limits<double>::quiet_NaN();
    EXPECT_EQ(Clamp<int32_t>(kNaN), 0);
}

TEST(ClampFloatingTest, PreservesNaNForFloatingTarget)
{
    const double kNaN   = std::numeric_limits<double>::quiet_NaN();
    const float  result = Clamp<float>(kNaN);
    EXPECT_TRUE(result != result);
}

TEST(AlignUpTest, ReturnsSameValueWhenAlreadyAligned) { EXPECT_EQ(AlignUp<uint32_t>(16u, 8u), 16u); }

TEST(AlignUpTest, AlignsUpWhenValueIsNotAligned) { EXPECT_EQ(AlignUp<uint32_t>(17u, 8u), 24u); }

TEST(AlignUpTest, ReturnsInputWhenAlignmentIsZero) { EXPECT_EQ(AlignUp<uint32_t>(17u, 0u), 17u); }

TEST(AlignUpTest, ClampsOnPositiveOverflow)
{
    EXPECT_EQ(AlignUp<uint32_t>(std::numeric_limits<uint32_t>::max() - 1u, 8u), std::numeric_limits<uint32_t>::max());
}

TEST(AlignUpTest, ReturnsSignedNegativeInputUnchanged) { EXPECT_EQ(AlignUp<int32_t>(-3, 8), -3); }

TEST(AlignDownTest, ReturnsSameValueWhenAlreadyAligned) { EXPECT_EQ(AlignDown<uint32_t>(16u, 8u), 16u); }

TEST(AlignDownTest, AlignsDownWhenValueIsNotAligned) { EXPECT_EQ(AlignDown<uint32_t>(17u, 8u), 16u); }

TEST(AlignDownTest, ReturnsInputWhenAlignmentIsZero) { EXPECT_EQ(AlignDown<uint32_t>(17u, 0u), 17u); }

TEST(AlignDownTest, ReturnsZeroWhenValueIsSmallerThanAlignment) { EXPECT_EQ(AlignDown<uint32_t>(3u, 8u), 0u); }

TEST(AlignDownTest, ReturnsSignedNegativeInputUnchanged) { EXPECT_EQ(AlignDown<int32_t>(-3, 8), -3); }

TEST(OptionalAddMixedIntegralTest, SubtractsFromUnsignedWhenDeltaIsNegative)
{
    EXPECT_EQ(OptionalAdd<size_t>(std::optional<size_t> {10}, -7, false), std::optional<size_t> {3});
}

TEST(OptionalAddMixedIntegralTest, ReturnsNulloptOnUnsignedUnderflowWithNegativeDelta)
{
    EXPECT_EQ(OptionalAdd<size_t>(std::optional<size_t> {5}, -7, false), std::nullopt);
}

TEST(OptionalAddMixedIntegralTest, ClampsUnsignedUnderflowToZeroWithNegativeDelta)
{
    EXPECT_EQ(OptionalAdd<size_t>(std::optional<size_t> {5}, -7, true), std::optional<size_t> {0});
}

TEST(OptionalAddMixedIntegralTest, AddsUnsignedDeltaToSignedValue)
{
    EXPECT_EQ(OptionalAdd<int32_t>(std::optional<int32_t> {-5}, 7u, false), std::optional<int32_t> {2});
}

TEST(OptionalAddMixedIntegralTest, AppliesNegativeSignedDeltaToSignedValue)
{
    EXPECT_EQ(OptionalAdd<int32_t>(std::optional<int32_t> {10}, -7, false), std::optional<int32_t> {3});
}

static_assert(OptionalConvert<int32_t>(std::optional<uint32_t> {42u}, false).has_value());
static_assert(OptionalConvert<int32_t>(std::optional<uint32_t> {42u}, false).value() == 42);
static_assert(OptionalConvert<int32_t>(std::optional<double> {1.5}, false).value() == 2);
static_assert(OptionalConvert<uint32_t>(std::optional<double> {-0.4}, false).value() == 0u);
static_assert(OptionalAdd<int32_t>(std::optional<int32_t> {10}, 5, false).value() == 15);
static_assert(Clamp<uint8_t>(static_cast<int32_t>(-1)) == 0);
static_assert(Clamp<int32_t>(1.5) == 2);
static_assert(AlignUp<uint32_t>(17u, 8u) == 24u);
static_assert(AlignDown<uint32_t>(17u, 8u) == 16u);
} // namespace xsdk::xbase::numbers
