#include "xbase.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

namespace xsdk::xbase::strings {
TEST(strings_tests, str_format)
{
    auto check = xbase::strings::StrFormat("%s is %zd val %.3f dbl", "Test", (size_t)789, 123.456);
    EXPECT_EQ(check, "Test is 789 val 123.456 dbl");
}

TEST(strings_tests, str_snake_case)
{
    auto check = xbase::strings::StrSnakeCase("camelCase");
    EXPECT_EQ(check, "camel_case");
    check = xbase::strings::StrSnakeCase("PascalCase");
    EXPECT_EQ(check, "pascal_case");
    check = xbase::strings::StrSnakeCase("MyNDISourceX");
    EXPECT_EQ(check, "my_ndi_source_x");
    check = xbase::strings::StrSnakeCase("AVDemux");
    EXPECT_EQ(check, "av_demux");
    check = xbase::strings::StrSnakeCase("MySource");
    EXPECT_EQ(check, "my_source");
    check = xbase::strings::StrSnakeCase("My_Source");
    EXPECT_EQ(check, "my_source");
    check = xbase::strings::StrSnakeCase("MY_Source");
    EXPECT_EQ(check, "my_source");
    check = xbase::strings::StrSnakeCase("my_source");
    EXPECT_EQ(check, "my_source");
}

TEST(strings_tests, str_split)
{
    auto check = xbase::strings::StrSplit("one, two, three\r,\nfour ,  ,\r\n,", ',', true, true);
    ASSERT_EQ(check.size(), 4);
    EXPECT_EQ(check[0], "one");
    EXPECT_EQ(check[1], "two");
    EXPECT_EQ(check[2], "three");
    EXPECT_EQ(check[3], "four");

    check = xbase::strings::StrSplit("one, two, three\r,\nfour ,  ,\r\n,", ',', true, false);
    ASSERT_EQ(check.size(), 7);
    EXPECT_EQ(check[0], "one");
    EXPECT_EQ(check[1], "two");
    EXPECT_EQ(check[2], "three");
    EXPECT_EQ(check[3], "four");
    EXPECT_EQ(check[5], "");
    EXPECT_EQ(check[6], "");

    check = xbase::strings::StrSplit("one, two, three\r,\nfour ,  ,\r \n,", ',', false, false);
    ASSERT_EQ(check.size(), 7);
    EXPECT_EQ(check[0], "one");
    EXPECT_EQ(check[1], " two");
    EXPECT_EQ(check[2], " three\r");
    EXPECT_EQ(check[3], "\nfour ");
    EXPECT_EQ(check[5], "\r \n");
    EXPECT_EQ(check[6], "");

    check = xbase::strings::StrSplit("", ',', false, true);
    ASSERT_EQ(check.size(), 0);
}

TEST(strings_tests, str_split_any)
{
    auto check = xbase::strings::StrSplitAny("one, two three\r,\nfour ,  ,\r\n,", ", \r\n", true, true);
    ASSERT_EQ(check.size(), 4);
    EXPECT_EQ(check[0], "one");
    EXPECT_EQ(check[1], "two");
    EXPECT_EQ(check[2], "three");
    EXPECT_EQ(check[3], "four");

    check = xbase::strings::StrSplitAny("one, two, three\r,\nfour ,  ,\r\n,", ", \r\n", true, false);
    ASSERT_EQ(check.size(), 16);
    EXPECT_EQ(check[0], "one");
    EXPECT_EQ(check[2], "two");
    EXPECT_EQ(check[4], "three");
    EXPECT_EQ(check[7], "four");
    EXPECT_EQ(check[5], "");
    EXPECT_EQ(check[6], "");

    check = xbase::strings::StrSplitAny("one, two, three\r,\nfour ,  ,\r \n,", ", \r\n", false, true);
    ASSERT_EQ(check.size(), 4);
    EXPECT_EQ(check[0], "one");
    EXPECT_EQ(check[1], "two");
    EXPECT_EQ(check[2], "three");
    EXPECT_EQ(check[3], "four");

    check = xbase::strings::StrSplitAny("one, two, three\r,\nfour ,  ,\r \n,", "", false, true);
    ASSERT_EQ(check.size(), 1);
    EXPECT_EQ(check[0], "one, two, three\r,\nfour ,  ,\r \n,");

    check = xbase::strings::StrSplitAny("", ", \r\n", false, true);
    ASSERT_EQ(check.size(), 0);
}

TEST(strings_tests, str_split_exact)
{
    auto check = xbase::strings::StrSplitExact("one::two::::three:four:::five", "::", true, true);
    ASSERT_EQ(check.size(), 4);
    EXPECT_EQ(check[0], "one");
    EXPECT_EQ(check[1], "two");
    EXPECT_EQ(check[2], "three:four");
    EXPECT_EQ(check[3], ":five");

    check = xbase::strings::StrSplitExact("one::two::::three:four:::five", "xxx", true, true);
    ASSERT_EQ(check.size(), 1);
    EXPECT_EQ(check[0], "one::two::::three:four:::five");

    check = xbase::strings::StrSplitExact("one::two::::three:four:::five", "", true, true);
    ASSERT_EQ(check.size(), 1);
    EXPECT_EQ(check[0], "one::two::::three:four:::five");
}

TEST(strings_tests, str_join)
{
    auto check = xbase::strings::StrJoin({"one", "two", "", "three"}, false);
    EXPECT_EQ(check, "onetwothree");

    check = xbase::strings::StrJoin({"one", "two", "", "three", ""}, false, "->");
    EXPECT_EQ(check, "one->two->->three->");

    check = xbase::strings::StrJoin({"", "one", "two", "", "three"}, false, "->");
    EXPECT_EQ(check, "->one->two->->three");

    check = xbase::strings::StrJoin({"", "", "one", "two", "", "three", "", ""}, true, "->");
    EXPECT_EQ(check, "one->two->three");
}

TEST(StringsTrim, EmptyAndWhitespaceOnly)
{
    EXPECT_EQ(StrTrim(""), "");
    EXPECT_EQ(StrTrim("   \t\r\n"), "");
}

TEST(StringsTrim, KeepsMiddleAndTrimsEnds)
{
    EXPECT_EQ(StrTrim("  abc  "), "abc");
    EXPECT_EQ(StrTrim("\t\nabc\r\n"), "abc");
    EXPECT_EQ(StrTrim("--abc--", "-"), "abc");
}

TEST(StringsSplit, Basic)
{
    const std::string s      = "a,b,c";
    const auto        tokens = StrSplit(s, ',');
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "a");
    EXPECT_EQ(tokens[1], "b");
    EXPECT_EQ(tokens[2], "c");
}

TEST(StringsSplit, KeepsEmptyByDefault)
{
    const std::string s      = ",a,,b,";
    const auto        tokens = StrSplit(s, ',');
    ASSERT_EQ(tokens.size(), 5u);
    EXPECT_EQ(tokens[0], "");
    EXPECT_EQ(tokens[1], "a");
    EXPECT_EQ(tokens[2], "");
    EXPECT_EQ(tokens[3], "b");
    EXPECT_EQ(tokens[4], "");
}

TEST(StringsSplit, TrimAndDropEmpty)
{
    const std::string s      = "  a , ,  b ,  ";
    const auto        tokens = StrSplit(s, ',', /*_trim_entries=*/true, /*_drop_empty=*/true);
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0], "a");
    EXPECT_EQ(tokens[1], "b");
}

TEST(StringsSplitAny, MultipleDelims)
{
    const std::string s      = "a,b; c\t d";
    const auto        tokens = StrSplitAny(s, ",;\t ", /*_trim_entries=*/true, /*_drop_empty=*/true);
    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokens[0], "a");
    EXPECT_EQ(tokens[1], "b");
    EXPECT_EQ(tokens[2], "c");
    EXPECT_EQ(tokens[3], "d");
}

TEST(StringsSplitExact, EmptyDelimReturnsWholeText)
{
    const std::string s      = "abc";
    const auto        tokens = StrSplitExact(s, "");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0], "abc");
}

TEST(StringsSplitExact, Basic)
{
    const std::string s      = "a--b----c--";
    const auto        tokens = StrSplitExact(s, "--");
    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokens[0], "a");
    EXPECT_EQ(tokens[1], "b");
    EXPECT_EQ(tokens[2], "");
    EXPECT_EQ(tokens[3], "c");
}

TEST(StringsSplitExact, TrimAndDropEmpty)
{
    const std::string s      = " a ::  :: b :: ";
    const auto        tokens = StrSplitExact(s, "::", /*_trim_entries=*/true, /*_drop_empty=*/true);
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0], "a");
    EXPECT_EQ(tokens[1], "b");
}

TEST(StringsCase, LowerUpper)
{
    EXPECT_EQ(StrLower("AbC_09"), "abc_09");
    EXPECT_EQ(StrUpper("AbC_09"), "ABC_09");
}

TEST(StringsCase, SnakeCase)
{
    EXPECT_EQ(StrSnakeCase("SimpleTest"), "simple_test");
    EXPECT_EQ(StrSnakeCase("simple_test"), "simple_test");
    EXPECT_EQ(StrSnakeCase("HTTPRequest"), "http_request");
    EXPECT_EQ(StrSnakeCase("HTTP"), "http");
    EXPECT_EQ(StrSnakeCase("A"), "a");
    EXPECT_EQ(StrSnakeCase("Already_Snake"), "already_snake");
}

TEST(StringsCompare, CaseInsensitive)
{
    EXPECT_EQ(StrCmpI("AbC", "aBc"), 0);
    EXPECT_LT(StrCmpI("abc", "abd"), 0);
    EXPECT_GT(StrCmpI("abe", "abd"), 0);
    EXPECT_LT(StrCmpI("a", "aa"), 0);
    EXPECT_GT(StrCmpI("aa", "a"), 0);
}

TEST(StringsCompare, CaseSensitiveFlag)
{
    EXPECT_EQ(StrCmp("AbC", "AbC", /*_case_sensitive=*/true), 0);
    EXPECT_NE(StrCmp("AbC", "aBc", /*_case_sensitive=*/true), 0);
    EXPECT_EQ(StrCmp("AbC", "aBc", /*_case_sensitive=*/false), 0);
}

TEST(StringsFind, CaseSensitiveAndInsensitive)
{
    const std::string s = "aBcDe";

    EXPECT_EQ(StrFind(s, "Bc", /*_case_sensitive=*/true, /*_offset=*/0), 1u);
    EXPECT_EQ(StrFind(s, "bc", /*_case_sensitive=*/true, /*_offset=*/0), std::string_view::npos);

    EXPECT_EQ(StrFind(s, "bc", /*_case_sensitive=*/false, /*_offset=*/0), 1u);
    EXPECT_EQ(StrFind(s, "DE", /*_case_sensitive=*/false, /*_offset=*/0), 3u);
    EXPECT_EQ(StrFind(s, "x", /*_case_sensitive=*/false, /*_offset=*/0), std::string_view::npos);

    EXPECT_EQ(StrFind(s, "bc", /*_case_sensitive=*/false, /*_offset=*/2), std::string_view::npos);
}

TEST(StringsPrefix, EmptyPrefixMatches)
{
    std::string rest;
    EXPECT_TRUE(StrIsPrefix("abc", "", &rest));
    // For empty prefix, rest isn't specified by contract; current impl keeps it untouched.
}

TEST(StringsPrefix, MatchesCaseInsensitive)
{
    std::string rest;
    EXPECT_TRUE(StrIsPrefix("AbCd", "aBc", &rest));
    EXPECT_EQ(rest, "d");

    const auto p = StrIsPrefix("AbCd", "aBc");
    EXPECT_TRUE(p.first);
    EXPECT_EQ(p.second, "d");

    EXPECT_FALSE(StrIsPrefix("AbCd", "zz", &rest));
}

TEST(StringsPostfix, MatchesCaseInsensitive)
{
    std::string rest;
    EXPECT_TRUE(StrIsPostfix("AbCd", "cD", &rest));
    EXPECT_EQ(rest, "Ab");

    const auto p = StrIsPostfix("AbCd", "cD");
    EXPECT_TRUE(p.first);
    EXPECT_EQ(p.second, "Ab");

    EXPECT_FALSE(StrIsPostfix("AbCd", "zz", &rest));
}

TEST(StringsJoin, Basic)
{
    const std::vector<std::string_view> parts = {"a", "", "b", "c"};

    EXPECT_EQ(StrJoin(parts, /*_drop_empty=*/false, ","), "a,,b,c");
    EXPECT_EQ(StrJoin(parts, /*_drop_empty=*/true, ","), "a,b,c");
    EXPECT_EQ(StrJoin(parts, /*_drop_empty=*/true, ""), "abc");
}

TEST(StringsFormat, Formats) { EXPECT_EQ(StrFormat("%s-%d", "id", 42), "id-42"); }

} // namespace xsdk::xbase::strings