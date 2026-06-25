#include "xbase/base64.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace xsdk::xbase::base64 {

TEST(Base64Test, EncodesAndDecodesString)
{
    const std::string input = "xBase base64 round trip";
    const auto        encoded = EncodeString(input);

    EXPECT_FALSE(encoded.empty());
    EXPECT_EQ(DecodeString(encoded), input);
}

TEST(Base64Test, EncodesAndDecodesBinaryData)
{
    const std::vector<uint8_t> input = {0x00, 0x01, 0x02, 0x7F, 0x80, 0xFE, 0xFF};
    const auto                 encoded = Encode(input);
    const auto                 decoded = Decode(encoded);

    EXPECT_EQ(decoded, input);
}

TEST(Base64Test, EmptyInputProducesEmptyOutput)
{
    EXPECT_TRUE(EncodeString("").empty());
    EXPECT_TRUE(DecodeString("").empty());
}

TEST(Base64Test, DecodesNonNullTerminatedView)
{
    const std::string encoded_with_suffix = EncodeString("payload") + "ignored";
    const auto        encoded             = std::string_view(encoded_with_suffix).substr(0, 12);

    EXPECT_EQ(DecodeString(encoded), "payload");
}

} // namespace xsdk::xbase::base64
