#include "xbase.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace xsdk::xbase::strings {
TEST(strings_tests, str_format)
{
    auto check = xbase::strings::Format("%s is %zd val %.3f dbl", "Test", (size_t)789, 123.456);
    EXPECT_EQ(check, "Test is 789 val 123.456 dbl");
}

TEST(strings_tests, str_snake_case)
{
    auto check = xbase::strings::SnakeCase("camelCase");
    EXPECT_EQ(check, "camel_case");
    check = xbase::strings::SnakeCase("PascalCase");
    EXPECT_EQ(check, "pascal_case");
    check = xbase::strings::SnakeCase("MyNDISourceX");
    EXPECT_EQ(check, "my_ndi_source_x");
    check = xbase::strings::SnakeCase("AVDemux");
    EXPECT_EQ(check, "av_demux");
    check = xbase::strings::SnakeCase("MySource");
    EXPECT_EQ(check, "my_source");
    check = xbase::strings::SnakeCase("My_Source");
    EXPECT_EQ(check, "my_source");
    check = xbase::strings::SnakeCase("MY_Source");
    EXPECT_EQ(check, "my_source");
    check = xbase::strings::SnakeCase("my_source");
    EXPECT_EQ(check, "my_source");
}

TEST(strings_tests, str_split)
{
    auto check = xbase::strings::Split("one, two, three\r,\nfour ,  ,\r\n,", ',', true, true);
    ASSERT_EQ(check.size(), 4);
    EXPECT_EQ(check[0], "one");
    EXPECT_EQ(check[1], "two");
    EXPECT_EQ(check[2], "three");
    EXPECT_EQ(check[3], "four");

    check = xbase::strings::Split("one, two, three\r,\nfour ,  ,\r\n,", ',', true, false);
    ASSERT_EQ(check.size(), 7);
    EXPECT_EQ(check[0], "one");
    EXPECT_EQ(check[1], "two");
    EXPECT_EQ(check[2], "three");
    EXPECT_EQ(check[3], "four");
    EXPECT_EQ(check[5], "");
    EXPECT_EQ(check[6], "");

    check = xbase::strings::Split("one, two, three\r,\nfour ,  ,\r \n,", ',', false, false);
    ASSERT_EQ(check.size(), 7);
    EXPECT_EQ(check[0], "one");
    EXPECT_EQ(check[1], " two");
    EXPECT_EQ(check[2], " three\r");
    EXPECT_EQ(check[3], "\nfour ");
    EXPECT_EQ(check[5], "\r \n");
    EXPECT_EQ(check[6], "");

    check = xbase::strings::Split("", ',', false, true);
    ASSERT_EQ(check.size(), 0);
}

TEST(strings_tests, str_split_any)
{
    auto check = xbase::strings::SplitAny("one, two three\r,\nfour ,  ,\r\n,", ", \r\n", true, true);
    ASSERT_EQ(check.size(), 4);
    EXPECT_EQ(check[0], "one");
    EXPECT_EQ(check[1], "two");
    EXPECT_EQ(check[2], "three");
    EXPECT_EQ(check[3], "four");

    check = xbase::strings::SplitAny("one, two, three\r,\nfour ,  ,\r\n,", ", \r\n", true, false);
    ASSERT_EQ(check.size(), 16);
    EXPECT_EQ(check[0], "one");
    EXPECT_EQ(check[2], "two");
    EXPECT_EQ(check[4], "three");
    EXPECT_EQ(check[7], "four");
    EXPECT_EQ(check[5], "");
    EXPECT_EQ(check[6], "");

    check = xbase::strings::SplitAny("one, two, three\r,\nfour ,  ,\r \n,", ", \r\n", false, true);
    ASSERT_EQ(check.size(), 4);
    EXPECT_EQ(check[0], "one");
    EXPECT_EQ(check[1], "two");
    EXPECT_EQ(check[2], "three");
    EXPECT_EQ(check[3], "four");

    check = xbase::strings::SplitAny("one, two, three\r,\nfour ,  ,\r \n,", "", false, true);
    ASSERT_EQ(check.size(), 1);
    EXPECT_EQ(check[0], "one, two, three\r,\nfour ,  ,\r \n,");

    check = xbase::strings::SplitAny("", ", \r\n", false, true);
    ASSERT_EQ(check.size(), 0);
}

TEST(strings_tests, str_split_exact)
{
    auto check = xbase::strings::SplitExact("one::two::::three:four:::five", "::", true, true);
    ASSERT_EQ(check.size(), 4);
    EXPECT_EQ(check[0], "one");
    EXPECT_EQ(check[1], "two");
    EXPECT_EQ(check[2], "three:four");
    EXPECT_EQ(check[3], ":five");

    check = xbase::strings::SplitExact("one::two::::three:four:::five", "xxx", true, true);
    ASSERT_EQ(check.size(), 1);
    EXPECT_EQ(check[0], "one::two::::three:four:::five");

    check = xbase::strings::SplitExact("one::two::::three:four:::five", "", true, true);
    ASSERT_EQ(check.size(), 1);
    EXPECT_EQ(check[0], "one::two::::three:four:::five");
}

TEST(strings_tests, str_join)
{
    auto check = xbase::strings::Join({"one", "two", "", "three"}, false);
    EXPECT_EQ(check, "onetwothree");

    check = xbase::strings::Join({"one", "two", "", "three", ""}, false, "->");
    EXPECT_EQ(check, "one->two->->three->");

    check = xbase::strings::Join({"", "one", "two", "", "three"}, false, "->");
    EXPECT_EQ(check, "->one->two->->three");

    check = xbase::strings::Join({"", "", "one", "two", "", "three", "", ""}, true, "->");
    EXPECT_EQ(check, "one->two->three");
}

TEST(StringsTrim, EmptyAndWhitespaceOnly)
{
    EXPECT_EQ(Trim(""), "");
    EXPECT_EQ(Trim("   \t\r\n"), "");
}

TEST(StringsTrim, KeepsMiddleAndTrimsEnds)
{
    EXPECT_EQ(Trim("  abc  "), "abc");
    EXPECT_EQ(Trim("\t\nabc\r\n"), "abc");
    EXPECT_EQ(Trim("--abc--", "-"), "abc");
}

TEST(StringsSplit, Basic)
{
    const std::string s      = "a,b,c";
    const auto        tokens = Split(s, ',');
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "a");
    EXPECT_EQ(tokens[1], "b");
    EXPECT_EQ(tokens[2], "c");
}

TEST(StringsSplit, KeepsEmptyByDefault)
{
    const std::string s      = ",a,,b,";
    const auto        tokens = Split(s, ',');
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
    const auto        tokens = Split(s, ',', /*_trim_entries=*/true, /*_drop_empty=*/true);
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0], "a");
    EXPECT_EQ(tokens[1], "b");
}

TEST(StringsSplitAny, MultipleDelims)
{
    const std::string s      = "a,b; c\t d";
    const auto        tokens = SplitAny(s, ",;\t ", /*_trim_entries=*/true, /*_drop_empty=*/true);
    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokens[0], "a");
    EXPECT_EQ(tokens[1], "b");
    EXPECT_EQ(tokens[2], "c");
    EXPECT_EQ(tokens[3], "d");
}

TEST(StringsSplitExact, EmptyDelimReturnsWholeText)
{
    const std::string s      = "abc";
    const auto        tokens = SplitExact(s, "");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0], "abc");
}

TEST(StringsSplitExactCopy, EmptyDelimReturnsWholeText)
{
    const std::string s      = "abc";
    const auto        tokens = SplitExactCopy(s, "");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0], "abc");
}

TEST(StringsSplitExact, Basic)
{
    const std::string s      = "a--b----c--";
    const auto        tokens = SplitExact(s, "--");
    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokens[0], "a");
    EXPECT_EQ(tokens[1], "b");
    EXPECT_EQ(tokens[2], "");
    EXPECT_EQ(tokens[3], "c");
}

TEST(StringsSplitExact, TrimAndDropEmpty)
{
    const std::string s      = " a ::  :: b :: ";
    const auto        tokens = SplitExact(s, "::", /*_trim_entries=*/true, /*_drop_empty=*/true);
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0], "a");
    EXPECT_EQ(tokens[1], "b");
}

TEST(StringsCase, LowerUpper)
{
    EXPECT_EQ(Lower("AbC_09"), "abc_09");
    EXPECT_EQ(Upper("AbC_09"), "ABC_09");
}

TEST(StringsCase, SnakeCase)
{
    EXPECT_EQ(SnakeCase("SimpleTest"), "simple_test");
    EXPECT_EQ(SnakeCase("simple_test"), "simple_test");
    EXPECT_EQ(SnakeCase("HTTPRequest"), "http_request");
    EXPECT_EQ(SnakeCase("HTTP"), "http");
    EXPECT_EQ(SnakeCase("A"), "a");
    EXPECT_EQ(SnakeCase("Already_Snake"), "already_snake");
}

TEST(StringsCompare, CaseInsensitive)
{
    EXPECT_EQ(CmpI("AbC", "aBc"), 0);
    EXPECT_LT(CmpI("abc", "abd"), 0);
    EXPECT_GT(CmpI("abe", "abd"), 0);
    EXPECT_LT(CmpI("a", "aa"), 0);
    EXPECT_GT(CmpI("aa", "a"), 0);
}

TEST(StringsCompare, CaseSensitiveFlag)
{
    EXPECT_EQ(Cmp("AbC", "AbC", /*_case_sensitive=*/true), 0);
    EXPECT_NE(Cmp("AbC", "aBc", /*_case_sensitive=*/true), 0);
    EXPECT_EQ(Cmp("AbC", "aBc", /*_case_sensitive=*/false), 0);
}

TEST(StringsFind, CaseSensitiveAndInsensitive)
{
    const std::string s = "aBcDe";

    EXPECT_EQ(Find(s, "Bc", /*_case_sensitive=*/true, /*_offset=*/0), 1u);
    EXPECT_EQ(Find(s, "bc", /*_case_sensitive=*/true, /*_offset=*/0), std::string_view::npos);

    EXPECT_EQ(Find(s, "bc", /*_case_sensitive=*/false, /*_offset=*/0), 1u);
    EXPECT_EQ(Find(s, "DE", /*_case_sensitive=*/false, /*_offset=*/0), 3u);
    EXPECT_EQ(Find(s, "x", /*_case_sensitive=*/false, /*_offset=*/0), std::string_view::npos);

    EXPECT_EQ(Find(s, "bc", /*_case_sensitive=*/false, /*_offset=*/2), std::string_view::npos);
}

TEST(StringsPrefix, EmptyPrefixMatches)
{
    std::string rest;
    EXPECT_TRUE(IsPrefix("abc", "", &rest));
    // For empty prefix, rest isn't specified by contract; current impl keeps it untouched.
}

TEST(StringsPrefix, MatchesCaseInsensitive)
{
    std::string rest;
    EXPECT_TRUE(IsPrefix("AbCd", "aBc", &rest));
    EXPECT_EQ(rest, "d");

    const auto p = IsPrefix("AbCd", "aBc");
    EXPECT_TRUE(p.first);
    EXPECT_EQ(p.second, "d");

    EXPECT_FALSE(IsPrefix("AbCd", "zz", &rest));
}

TEST(StringsPostfix, MatchesCaseInsensitive)
{
    std::string rest;
    EXPECT_TRUE(IsPostfix("AbCd", "cD", &rest));
    EXPECT_EQ(rest, "Ab");

    const auto p = IsPostfix("AbCd", "cD");
    EXPECT_TRUE(p.first);
    EXPECT_EQ(p.second, "Ab");

    EXPECT_FALSE(IsPostfix("AbCd", "zz", &rest));
}

TEST(StringsJoin, Basic)
{
    const std::vector<std::string_view> parts = {"a", "", "b", "c"};

    EXPECT_EQ(Join(parts, /*_drop_empty=*/false, ","), "a,,b,c");
    EXPECT_EQ(Join(parts, /*_drop_empty=*/true, ","), "a,b,c");
    EXPECT_EQ(Join(parts, /*_drop_empty=*/true, ""), "abc");
}

TEST(StringsFormat, Formats) { EXPECT_EQ(Format("%s-%d", "id", 42), "id-42"); }

TEST(StringsFiles, SaveFileReportsOpenFailure)
{
    std::string error;
    EXPECT_FALSE(SaveFile("content", std::filesystem::temp_directory_path().string(), std::ios_base::out, &error));
    EXPECT_FALSE(error.empty());
}

TEST(StringsFiles, LoadAndSaveAcceptNonNullTerminatedFilenameView)
{
    const auto        path_text = (std::filesystem::temp_directory_path() / "xbase_strings_file_view_test.txt").string();
    const std::string decorated_path = path_text + ".ignored";
    const auto        filename       = std::string_view(decorated_path).substr(0, path_text.size());

    ASSERT_TRUE(SaveFile("payload", filename));
    EXPECT_EQ(LoadFile(filename), "payload");
    std::filesystem::remove(path_text);
}

} // namespace xsdk::xbase::strings
