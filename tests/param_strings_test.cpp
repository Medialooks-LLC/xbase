#include "xbase/param_strings.h"

#include <string>

#include <gtest/gtest.h>

namespace xsdk::xbase {
namespace param_strings {

    TEST(ParseParamStringTest, ReturnsNullInputErrorForNullptr)
    {
        const auto result = ParseParamString(static_cast<const char*>(nullptr));

        ASSERT_TRUE(result.items.empty());
        ASSERT_TRUE(result.flags.empty());
        ASSERT_EQ(result.errors.size(), 1U);

        EXPECT_EQ(result.errors[0].code, ParamParseErrorCode::kNullInput);
        EXPECT_EQ(result.errors[0].position, 0U);
    }

    TEST(ParseParamStringTest, EmptyStringProducesEmptySuccessfulResult)
    {
        const auto result = ParseParamString(std::string_view(""));

        EXPECT_TRUE(result.items.empty());
        EXPECT_TRUE(result.flags.empty());
        EXPECT_TRUE(result.errors.empty());
        EXPECT_TRUE(result.Empty());
    }

    TEST(ParseParamStringTest, DefaultModeCopiesInput)
    {
        const std::string input = "format=mp4";

        const auto result = ParseParamString(std::string_view(input));

        ASSERT_EQ(result.items.size(), 1U);
        ASSERT_TRUE(result.StorageShared());
        EXPECT_FALSE(result.StorageShared()->empty());

        EXPECT_EQ(result.items[0].key.text, "format");
        EXPECT_EQ(result.items[0].value.text, "mp4");

        EXPECT_EQ(result.items[0].key.text.data(), result.StorageShared()->data() + result.items[0].key.begin);
        EXPECT_EQ(result.items[0].value.text.data(), result.StorageShared()->data() + result.items[0].value.begin);
    }

    TEST(ParseParamStringTest, CanWorkInZeroCopyMode)
    {
        const std::string input  = "format=mp4";
        const auto        result = ParseParamString(std::string_view(input), ParamParseOptions(false));

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_FALSE(result.StorageShared());

        EXPECT_EQ(result.items[0].key.text, "format");
        EXPECT_EQ(result.items[0].value.text, "mp4");

        EXPECT_EQ(result.items[0].key.text.data(), input.data() + result.items[0].key.begin);
        EXPECT_EQ(result.items[0].value.text.data(), input.data() + result.items[0].value.begin);
    }

    TEST(ParseParamStringTest, ParsesSingleUnquotedItem)
    {
        const auto result = ParseParamString("format=mp4");

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_TRUE(result.flags.empty());
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "format");
        EXPECT_EQ(result.items[0].value.text, "mp4");
    }

    TEST(ParseParamStringTest, ParsesMultipleItems)
    {
        const auto result = ParseParamString(
            "format='mp4' video::codec='q264sw' video::bf='1' audio::codec='ac3_fixed'");

        ASSERT_EQ(result.items.size(), 4U);
        EXPECT_TRUE(result.flags.empty());
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "format");
        EXPECT_EQ(result.items[0].value.text, "mp4");

        EXPECT_EQ(result.items[1].key.text, "video::codec");
        EXPECT_EQ(result.items[1].value.text, "q264sw");

        EXPECT_EQ(result.items[2].key.text, "video::bf");
        EXPECT_EQ(result.items[2].value.text, "1");

        EXPECT_EQ(result.items[3].key.text, "audio::codec");
        EXPECT_EQ(result.items[3].value.text, "ac3_fixed");
    }

    TEST(ParseParamStringTest, AllowsWhitespaceAroundEquals)
    {
        const auto result = ParseParamString("a=1 b =2 c= 3 d = 4");

        ASSERT_EQ(result.items.size(), 4U);
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "a");
        EXPECT_EQ(result.items[0].value.text, "1");

        EXPECT_EQ(result.items[1].key.text, "b");
        EXPECT_EQ(result.items[1].value.text, "2");

        EXPECT_EQ(result.items[2].key.text, "c");
        EXPECT_EQ(result.items[2].value.text, "3");

        EXPECT_EQ(result.items[3].key.text, "d");
        EXPECT_EQ(result.items[3].value.text, "4");
    }

    TEST(ParseParamStringTest, ParsesFlagsWithoutEquals)
    {
        const auto result = ParseParamString("faststart realtime lowlatency");

        EXPECT_TRUE(result.items.empty());
        ASSERT_EQ(result.flags.size(), 3U);
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.flags[0].key.text, "faststart");
        EXPECT_EQ(result.flags[1].key.text, "realtime");
        EXPECT_EQ(result.flags[2].key.text, "lowlatency");
    }

    TEST(ParseParamStringTest, ParsesMixedItemsAndFlags)
    {
        const auto result = ParseParamString("faststart format=mp4 realtime video::b=10M");

        ASSERT_EQ(result.items.size(), 2U);
        ASSERT_EQ(result.flags.size(), 2U);
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.flags[0].key.text, "faststart");
        EXPECT_EQ(result.items[0].key.text, "format");
        EXPECT_EQ(result.items[0].value.text, "mp4");
        EXPECT_EQ(result.flags[1].key.text, "realtime");
        EXPECT_EQ(result.items[1].key.text, "video::b");
        EXPECT_EQ(result.items[1].value.text, "10M");
    }

    TEST(ParseParamStringTest, SupportsEmptyValueAtEnd)
    {
        const auto result = ParseParamString("a=");

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_TRUE(result.flags.empty());
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "a");
        EXPECT_TRUE(result.items[0].value.text.empty());
        EXPECT_TRUE(result.items[0].value.Empty());
    }

    TEST(ParseParamStringTest, SupportsEmptySingleQuotedValue)
    {
        const auto result = ParseParamString("a=''");

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "a");
        EXPECT_TRUE(result.items[0].value.text.empty());
    }

    TEST(ParseParamStringTest, SupportsEmptyDoubleQuotedValue)
    {
        const auto result = ParseParamString("a=\"\"");

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "a");
        EXPECT_TRUE(result.items[0].value.text.empty());
    }

    TEST(ParseParamStringTest, ParsesSingleQuotedValueWithoutOuterQuotes)
    {
        const auto result = ParseParamString("a='hello world'");

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "a");
        EXPECT_EQ(result.items[0].value.text, "hello world");
    }

    TEST(ParseParamStringTest, ParsesDoubleQuotedValueWithoutOuterQuotes)
    {
        const auto result = ParseParamString("a=\"hello world\"");

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "a");
        EXPECT_EQ(result.items[0].value.text, "hello world");
    }

    TEST(ParseParamStringTest, KeepsDifferentQuoteTypeInsideSingleQuotedValue)
    {
        const auto result = ParseParamString("a='say \"hello\" now'");

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_TRUE(result.errors.empty());
        EXPECT_EQ(result.items[0].value.text, "say \"hello\" now");
    }

    TEST(ParseParamStringTest, KeepsDifferentQuoteTypeInsideDoubleQuotedValue)
    {
        const auto result = ParseParamString("a=\"say 'hello' now\"");

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_TRUE(result.errors.empty());
        EXPECT_EQ(result.items[0].value.text, "say 'hello' now");
    }

    TEST(ParseParamStringTest, EscapedSingleQuoteDoesNotCloseSingleQuotedValue)
    {
        const auto result = ParseParamString("a='it\\'s ok' b=2");

        ASSERT_EQ(result.items.size(), 2U);
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "a");
        EXPECT_EQ(result.items[0].value.text, "it\\'s ok");

        EXPECT_EQ(result.items[1].key.text, "b");
        EXPECT_EQ(result.items[1].value.text, "2");
    }

    TEST(ParseParamStringTest, EscapedDoubleQuoteDoesNotCloseDoubleQuotedValue)
    {
        const auto result = ParseParamString("a=\"a\\\"b\" c=3");

        ASSERT_EQ(result.items.size(), 2U);
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "a");
        EXPECT_EQ(result.items[0].value.text, "a\\\"b");

        EXPECT_EQ(result.items[1].key.text, "c");
        EXPECT_EQ(result.items[1].value.text, "3");
    }

    TEST(ParseParamStringTest, BackslashSkipsAnyNextCharacterInsideQuotedValue)
    {
        const auto result = ParseParamString("a=\"x\\qz\" b=2");

        ASSERT_EQ(result.items.size(), 2U);
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].value.text, "x\\qz");
        EXPECT_EQ(result.items[1].key.text, "b");
        EXPECT_EQ(result.items[1].value.text, "2");
    }

    TEST(ParseParamStringTest, UnterminatedSingleQuotedValueProducesError)
    {
        const auto result = ParseParamString("a='abc");

        EXPECT_TRUE(result.items.empty());
        EXPECT_TRUE(result.flags.empty());
        ASSERT_EQ(result.errors.size(), 1U);

        EXPECT_EQ(result.errors[0].code, ParamParseErrorCode::kUnterminatedQuotedValue);
        EXPECT_EQ(result.errors[0].position, 2U);
        EXPECT_EQ(result.errors[0].key.text, "a");
        EXPECT_EQ(result.errors[0].token.text, "'abc");
    }

    TEST(ParseParamStringTest, UnterminatedDoubleQuotedValueProducesError)
    {
        const auto result = ParseParamString("a=\"abc");

        EXPECT_TRUE(result.items.empty());
        EXPECT_TRUE(result.flags.empty());
        ASSERT_EQ(result.errors.size(), 1U);

        EXPECT_EQ(result.errors[0].code, ParamParseErrorCode::kUnterminatedQuotedValue);
        EXPECT_EQ(result.errors[0].position, 2U);
        EXPECT_EQ(result.errors[0].key.text, "a");
        EXPECT_EQ(result.errors[0].token.text, "\"abc");
    }

    TEST(ParseParamStringTest, EqualSignWithoutKeyProducesErrorAndParserContinues)
    {
        const auto result = ParseParamString("=abc good=1");

        ASSERT_EQ(result.items.size(), 1U);
        ASSERT_EQ(result.errors.size(), 1U);

        EXPECT_EQ(result.errors[0].code, ParamParseErrorCode::kEmptyKey);
        EXPECT_EQ(result.errors[0].position, 0U);

        EXPECT_EQ(result.items[0].key.text, "good");
        EXPECT_EQ(result.items[0].value.text, "1");
    }

    TEST(ParseParamStringTest, EqualSignWithWhitespaceWithoutKeyProducesErrorAndParserContinues)
    {
        const auto result = ParseParamString(" = abc good=1");

        ASSERT_EQ(result.items.size(), 1U);
        ASSERT_EQ(result.errors.size(), 1U);

        EXPECT_EQ(result.errors[0].code, ParamParseErrorCode::kEmptyKey);
        EXPECT_EQ(result.items[0].key.text, "good");
        EXPECT_EQ(result.items[0].value.text, "1");
    }

    TEST(ParseParamStringTest, PreservesTokenOffsetsForUnquotedValue)
    {
        const std::string input  = "  aa = bb  ";
        const auto        result = ParseParamString(std::string_view(input), ParamParseOptions(false));

        ASSERT_EQ(result.items.size(), 1U);

        EXPECT_EQ(result.items[0].key.begin, 2U);
        EXPECT_EQ(result.items[0].key.end, 4U);
        EXPECT_EQ(result.items[0].key.text, "aa");

        EXPECT_EQ(result.items[0].value.begin, 7U);
        EXPECT_EQ(result.items[0].value.end, 9U);
        EXPECT_EQ(result.items[0].value.text, "bb");
    }

    TEST(ParseParamStringTest, PreservesTokenOffsetsForQuotedValue)
    {
        const std::string input  = "a='xyz' flag";
        const auto        result = ParseParamString(std::string_view(input), ParamParseOptions(false));

        ASSERT_EQ(result.items.size(), 1U);
        ASSERT_EQ(result.flags.size(), 1U);

        EXPECT_EQ(result.items[0].key.begin, 0U);
        EXPECT_EQ(result.items[0].key.end, 1U);
        EXPECT_EQ(result.items[0].value.begin, 3U);
        EXPECT_EQ(result.items[0].value.end, 6U);
        EXPECT_EQ(result.items[0].value.text, "xyz");

        EXPECT_EQ(result.flags[0].key.begin, 8U);
        EXPECT_EQ(result.flags[0].key.end, 12U);
        EXPECT_EQ(result.flags[0].key.text, "flag");
    }

    TEST(ParseParamStringTest, UnquotedValueConsumesUntilWhitespace)
    {
        const auto result = ParseParamString("a=b=1 c=2");

        ASSERT_EQ(result.items.size(), 2U);
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "a");
        EXPECT_EQ(result.items[0].value.text, "b=1");

        EXPECT_EQ(result.items[1].key.text, "c");
        EXPECT_EQ(result.items[1].value.text, "2");
    }

    TEST(ParseParamStringTest, HandlesWhitespaceOnlyInput)
    {
        const auto result = ParseParamString(" \t \n \r ");

        EXPECT_TRUE(result.items.empty());
        EXPECT_TRUE(result.flags.empty());
        EXPECT_TRUE(result.errors.empty());
    }

    TEST(ParseParamStringTest, ParsesComplexExample)
    {
        const auto result = ParseParamString(
            "format='mp4' video::codec='q264sw' video::bf='1' "
            "video::convergence='111' video::b='10M' audio::codec='ac3_fixed'");

        ASSERT_EQ(result.items.size(), 6U);
        EXPECT_TRUE(result.flags.empty());
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "format");
        EXPECT_EQ(result.items[0].value.text, "mp4");

        EXPECT_EQ(result.items[1].key.text, "video::codec");
        EXPECT_EQ(result.items[1].value.text, "q264sw");

        EXPECT_EQ(result.items[2].key.text, "video::bf");
        EXPECT_EQ(result.items[2].value.text, "1");

        EXPECT_EQ(result.items[3].key.text, "video::convergence");
        EXPECT_EQ(result.items[3].value.text, "111");

        EXPECT_EQ(result.items[4].key.text, "video::b");
        EXPECT_EQ(result.items[4].value.text, "10M");

        EXPECT_EQ(result.items[5].key.text, "audio::codec");
        EXPECT_EQ(result.items[5].value.text, "ac3_fixed");
    }

    TEST(ParseParamStringTest, ParsesAdjacentTokenAfterSingleQuotedValue)
    {
        const auto result = ParseParamString("a='x'b=1");

        ASSERT_EQ(result.items.size(), 2U);
        EXPECT_TRUE(result.flags.empty());
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "a");
        EXPECT_EQ(result.items[0].value.text, "x");
        EXPECT_EQ(result.items[1].key.text, "b");
        EXPECT_EQ(result.items[1].value.text, "1");
    }

    TEST(ParseParamStringTest, ParsesAdjacentTokenAfterDoubleQuotedValue)
    {
        const auto result = ParseParamString("a=\"x\"b=1");

        ASSERT_EQ(result.items.size(), 2U);
        EXPECT_TRUE(result.flags.empty());
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "a");
        EXPECT_EQ(result.items[0].value.text, "x");
        EXPECT_EQ(result.items[1].key.text, "b");
        EXPECT_EQ(result.items[1].value.text, "1");
    }

    TEST(ParseParamStringTest, SpaceAfterEqualsBeforeNextAssignmentMeansEmptyValue)
    {
        const auto result = ParseParamString("a= b=2");

        ASSERT_EQ(result.items.size(), 2U);
        EXPECT_TRUE(result.flags.empty());
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "a");
        EXPECT_TRUE(result.items[0].value.text.empty());

        EXPECT_EQ(result.items[1].key.text, "b");
        EXPECT_EQ(result.items[1].value.text, "2");
    }

    TEST(ParseParamStringTest, SpaceAfterEqualsBeforePlainValueIsAllowed)
    {
        const auto result = ParseParamString("a = 1");

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_TRUE(result.flags.empty());
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "a");
        EXPECT_EQ(result.items[0].value.text, "1");
    }

    TEST(ParseParamStringTest, ParsesDoubleQuotedKey)
    {
        const auto result = ParseParamString("\"video::codec\"='q264sw'");

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_TRUE(result.flags.empty());
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "video::codec");
        EXPECT_EQ(result.items[0].value.text, "q264sw");
    }

    TEST(ParseParamStringTest, ParsesSingleQuotedKey)
    {
        const auto result = ParseParamString("'video::codec'='q264sw'");

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_TRUE(result.flags.empty());
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "video::codec");
        EXPECT_EQ(result.items[0].value.text, "q264sw");
    }

    TEST(ParseParamStringTest, ParsesQuotedFlag)
    {
        const auto result = ParseParamString("'fast start'");

        EXPECT_TRUE(result.items.empty());
        ASSERT_EQ(result.flags.size(), 1U);
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.flags[0].key.text, "fast start");
    }

    TEST(ParseParamStringTest, EmptyQuotedKeyProducesErrorAndParserContinues)
    {
        const auto result = ParseParamString("\"\"=value good=1");

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_TRUE(result.flags.empty());
        ASSERT_EQ(result.errors.size(), 1U);

        EXPECT_EQ(result.errors[0].code, ParamParseErrorCode::kEmptyKey);
        EXPECT_EQ(result.items[0].key.text, "good");
        EXPECT_EQ(result.items[0].value.text, "1");
    }

    TEST(ParseParamStringTest, UnterminatedQuotedKeyProducesError)
    {
        const auto result = ParseParamString("\"abc=1");

        EXPECT_TRUE(result.items.empty());
        EXPECT_TRUE(result.flags.empty());
        ASSERT_EQ(result.errors.size(), 1U);

        EXPECT_EQ(result.errors[0].code, ParamParseErrorCode::kUnterminatedQuotedKey);
        EXPECT_EQ(result.errors[0].position, 0U);
    }

    TEST(ParseParamStringTest, UnexpectedCharacterAfterQuotedKeyProducesError)
    {
        const auto result = ParseParamString("\"foo\"x=1 good=2");

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_TRUE(result.flags.empty());
        ASSERT_EQ(result.errors.size(), 1U);

        EXPECT_EQ(result.errors[0].code, ParamParseErrorCode::kUnexpectedCharacter);
        EXPECT_EQ(result.errors[0].position, 5U);

        EXPECT_EQ(result.items[0].key.text, "good");
        EXPECT_EQ(result.items[0].value.text, "2");
    }

    TEST(ParseParamStringTest, EmptyKeyWithWhitespaceDoesNotConsumeNextAssignment)
    {
        const auto result = ParseParamString("= foo=1 bar=2");

        ASSERT_EQ(result.items.size(), 2U);
        EXPECT_TRUE(result.flags.empty());
        ASSERT_EQ(result.errors.size(), 1U);

        EXPECT_EQ(result.errors[0].code, ParamParseErrorCode::kEmptyKey);

        EXPECT_EQ(result.items[0].key.text, "foo");
        EXPECT_EQ(result.items[0].value.text, "1");

        EXPECT_EQ(result.items[1].key.text, "bar");
        EXPECT_EQ(result.items[1].value.text, "2");
    }

    TEST(ParseParamStringTest, QuotedAssignmentAfterSpaceAfterEqualsMeansEmptyValue)
    {
        const auto result = ParseParamString("a= \"b\"=2");

        ASSERT_EQ(result.items.size(), 2U);
        EXPECT_TRUE(result.flags.empty());
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "a");
        EXPECT_TRUE(result.items[0].value.text.empty());

        EXPECT_EQ(result.items[1].key.text, "b");
        EXPECT_EQ(result.items[1].value.text, "2");
    }

    TEST(ParseParamStringTest, AccumulatesMultipleEmptyKeyErrorsAndContinues)
    {
        const auto result = ParseParamString("=a =b good=1");

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_TRUE(result.flags.empty());
        ASSERT_EQ(result.errors.size(), 2U);

        EXPECT_EQ(result.errors[0].code, ParamParseErrorCode::kEmptyKey);
        EXPECT_EQ(result.errors[0].position, 0U);

        EXPECT_EQ(result.errors[1].code, ParamParseErrorCode::kEmptyKey);
        EXPECT_EQ(result.errors[1].position, 3U);

        EXPECT_EQ(result.items[0].key.text, "good");
        EXPECT_EQ(result.items[0].value.text, "1");
    }

    TEST(ParseParamStringTest, UnterminatedQuotedValueStopsParsingAfterError)
    {
        const auto result = ParseParamString("a='unterminated b=2");

        EXPECT_TRUE(result.items.empty());
        EXPECT_TRUE(result.flags.empty());
        ASSERT_EQ(result.errors.size(), 1U);

        EXPECT_EQ(result.errors[0].code, ParamParseErrorCode::kUnterminatedQuotedValue);
        EXPECT_EQ(result.errors[0].position, 2U);
        EXPECT_EQ(result.errors[0].key.text, "a");
        EXPECT_EQ(result.errors[0].token.text, "'unterminated b=2");
    }

    TEST(ParseParamStringTest, DoubleBackslashInsideSingleQuotedValueAllowsClosingQuote)
    {
        const auto result = ParseParamString("a='x\\\\' b=2");

        ASSERT_EQ(result.items.size(), 2U);
        EXPECT_TRUE(result.flags.empty());
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.text, "a");
        EXPECT_EQ(result.items[0].value.text, "x\\\\");
        EXPECT_EQ(result.items[1].key.text, "b");
        EXPECT_EQ(result.items[1].value.text, "2");
    }

    TEST(ParseParamStringTest, BackslashBeforeClosingSingleQuoteMakesValueUnterminated)
    {
        const auto result = ParseParamString("a='x\\'");

        EXPECT_TRUE(result.items.empty());
        EXPECT_TRUE(result.flags.empty());
        ASSERT_EQ(result.errors.size(), 1U);

        EXPECT_EQ(result.errors[0].code, ParamParseErrorCode::kUnterminatedQuotedValue);
        EXPECT_EQ(result.errors[0].position, 2U);
        EXPECT_EQ(result.errors[0].key.text, "a");
        EXPECT_EQ(result.errors[0].token.text, "'x\\'");
    }

    TEST(ParseParamStringTest, DefaultModeHandlesDefaultEmptyStringView)
    {
        const auto result = ParseParamString(std::string_view {});

        EXPECT_TRUE(result.items.empty());
        EXPECT_TRUE(result.flags.empty());
        EXPECT_TRUE(result.errors.empty());
        EXPECT_TRUE(result.OwnsInput());
        ASSERT_TRUE(result.StorageShared());
        EXPECT_TRUE(result.StorageShared()->empty());
    }

    TEST(ParseParamStringTest, PreservesTokenOffsetsForQuotedKey)
    {
        const std::string input  = "\"video::codec\"=q264sw";
        const auto        result = ParseParamString(std::string_view(input), ParamParseOptions(false));

        ASSERT_EQ(result.items.size(), 1U);
        EXPECT_TRUE(result.errors.empty());

        EXPECT_EQ(result.items[0].key.begin, 1U);
        EXPECT_EQ(result.items[0].key.end, 13U);
        EXPECT_EQ(result.items[0].key.text, "video::codec");

        EXPECT_EQ(result.items[0].value.begin, 15U);
        EXPECT_EQ(result.items[0].value.end, 21U);
        EXPECT_EQ(result.items[0].value.text, "q264sw");
    }

} // namespace param_strings
} // namespace xsdk::xbase
