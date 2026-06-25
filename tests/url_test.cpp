#include "xbase/url.h"

#include <gtest/gtest.h>

namespace xsdk::xbase::url {

TEST(UrlMake, UsesSlashBeforePath)
{
    Parts parts;
    parts.protocol = "http://";
    parts.server   = "example.com";
    parts.path     = "media/file.mov";

    EXPECT_EQ(Make(parts), "http://example.com/media/file.mov");
}

TEST(UrlParseMake, PreservesPathSeparator)
{
    const auto parsed = Parse("http://user:pass@example.com:8080/media/file.mov?b=2&a=1");
    const auto made   = Make(parsed);

    EXPECT_NE(made.find("/media/file.mov"), std::string::npos);
    EXPECT_EQ(made.find(":media/file.mov"), std::string::npos);
}

} // namespace xsdk::xbase::url
