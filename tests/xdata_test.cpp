#include "xbase.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <thread>

// TEMP
// #include "../../include/xmodules/Struct_Media.h"
// #include "../../include/xutils/utils_vectors.h"

using namespace xsdk;

// NOLINTBEGIN(*)

TEST(xdata_tests, data_set)
{
    auto data_sp = xdata::Create();

    std::string str = "my test";
    auto        idx = xdata::Set(data_sp.get(), -1, str);
    EXPECT_EQ(idx, 0);
    idx = xdata::Set(data_sp.get(), -1, std::string("second_zzz"), -1);
    EXPECT_EQ(idx, 1);
    idx = xdata::Set(data_sp.get(), -1, std::string("third"));
    EXPECT_EQ(idx, 2);
    idx = xdata::Set(data_sp.get(), 1, std::string("second"));
    EXPECT_EQ(idx, 1);

    EXPECT_EQ(xdata::Count<std::string>(data_sp.get()), 3);

    auto str_sp = xdata::Get<std::string>(data_sp.get());
    ASSERT_TRUE(str_sp);
    EXPECT_EQ(*str_sp, str);

    str_sp = xdata::Get<std::string>(data_sp.get(), 1);
    ASSERT_TRUE(str_sp);
    EXPECT_EQ(*str_sp, "second");

    str_sp = xdata::Get<std::string>(data_sp.get(), 2);
    ASSERT_TRUE(str_sp);
    EXPECT_EQ(*str_sp, "third");
}

TEST(xdata_tests, data_clone_exclude)
{
    auto data_sp = xdata::Create();

    std::string str_val = "my test";
    auto        idx = xdata::Set(data_sp.get(), -1, str_val);
    EXPECT_EQ(idx, 0);
    int64_t int_val = 123;
    idx = xdata::Set(data_sp.get(), -1, int_val);
    EXPECT_EQ(idx, 0);
    double double_val = 1.23;
    idx     = xdata::Set(data_sp.get(), -1, double_val);
    EXPECT_EQ(idx, 0);

    EXPECT_EQ(xdata::Count<std::string>(data_sp.get()), 1);
    EXPECT_EQ(xdata::Count<int64_t>(data_sp.get()), 1);
    EXPECT_EQ(xdata::Count<double>(data_sp.get()), 1);

    std::set<uint64_t> types {xbase::TypeUid<int64_t>(), xbase::TypeUid<double>()};
    auto clone_sp = data_sp->Clone(types, IData::CloneSetType::Exclude);

    EXPECT_EQ(xdata::Count<std::string>(clone_sp.get()), 1);
    EXPECT_EQ(xdata::Count<int64_t>(clone_sp.get()), 0);
    EXPECT_EQ(xdata::Count<double>(clone_sp.get()), 0);
}

TEST(xdata_tests, data_clone_include)
{
    auto data_sp = xdata::Create();

    std::string str_val = "my test";
    auto        idx     = xdata::Set(data_sp.get(), -1, str_val);
    EXPECT_EQ(idx, 0);
    int64_t int_val = 123;
    idx             = xdata::Set(data_sp.get(), -1, int_val);
    EXPECT_EQ(idx, 0);
    double double_val = 1.23;
    idx               = xdata::Set(data_sp.get(), -1, double_val);
    EXPECT_EQ(idx, 0);

    EXPECT_EQ(xdata::Count<std::string>(data_sp.get()), 1);
    EXPECT_EQ(xdata::Count<int64_t>(data_sp.get()), 1);
    EXPECT_EQ(xdata::Count<double>(data_sp.get()), 1);

    std::set<uint64_t> types {xbase::TypeUid<int64_t>(), xbase::TypeUid<double>()};
    auto               clone_sp = data_sp->Clone(types, IData::CloneSetType::Include);

    EXPECT_EQ(xdata::Count<std::string>(clone_sp.get()), 0);
    EXPECT_EQ(xdata::Count<int64_t>(clone_sp.get()), 1);
    EXPECT_EQ(xdata::Count<double>(clone_sp.get()), 1);
}

// TEST(xdata_tests, data_set_holder)
//{
//     auto data_sp = std::make_shared<XDataImpl>();
//
//     XFORMAT_V xFormatV  = {kAvPixFmtAbgr , 720, 576};
//     XFORMAT_A xFormatA = {kAvSampleFmtFlt, 48000, 16};
//     XFORMAT_A xFormatA2 = {kAvSampleFmtFltp, 32000, 8};
//     XTIME     xTime    = {eXTF_Packet_Discard, 200};
//
//     std::vector<uint8_t> holder(128);
//     std::memset(holder.data(), 77, holder.size());
//
//     auto idx = xdata::Set(data_sp.get(), 0, xFormatV, std::move(holder));
//     //EXPECT_EQ(holder.size(), 0); // clang error !
//     EXPECT_EQ(idx, 0);
//     idx = xdata::Set(data_sp.get(), 0, xFormatA);
//     EXPECT_EQ(idx, 0);
//     idx = xdata::Set(data_sp.get(), -1, XFORMAT_A(xFormatA2));
//     EXPECT_EQ(idx, 1);
//     idx = xdata::Set(data_sp.get(), 0, xTime);
//     EXPECT_EQ(idx, 0);
//
//     EXPECT_EQ(xdata::Count<xsdk::XFORMAT_V>(data_sp), 1);
//     EXPECT_EQ(xdata::Count<xsdk::XFORMAT_A>(data_sp), 2);
//     EXPECT_EQ(xdata::Count<xsdk::XTIME>(data_sp), 1);
//
//     auto [fmt_v_p, hld_p] = XDataGetWithHolder<xsdk::XFORMAT_V, std::vector<uint8_t>>(data_sp);
//     ASSERT_TRUE(fmt_v_p);
//     EXPECT_TRUE(fmt_v_p->IsEqual(xFormatV, false));
//     ASSERT_TRUE(hld_p);
//     EXPECT_EQ(hld_p->Size(), 128);
//     EXPECT_EQ(hld_p->At(17), 77);
//
//     auto fmt_a_p  = xdata::Get<XFORMAT_A>(data_sp.get(), 0);
//     auto fmt_a_p2 = xdata::Get<XFORMAT_A>(data_sp.get(), 1);
//     ASSERT_TRUE(fmt_a_p);
//     ASSERT_TRUE(fmt_a_p2);
//
//     EXPECT_TRUE(fmt_a_p->IsEqual(xFormatA));
//     EXPECT_TRUE(fmt_a_p2->IsEqual(xFormatA2));
//
//     idx = xdata::Set(data_sp.get(), 1, XFORMAT_A(xFormatA));
//     EXPECT_EQ(idx, 1);
//
//     auto fmt_a_p3 = xdata::Get<XFORMAT_A>(data_sp.get(), 1);
//     ASSERT_TRUE(fmt_a_p3);
//     EXPECT_TRUE(fmt_a_p3->IsEqual(xFormatA));
//
//
//
//
//     {
//         std::vector<uint8_t> holder2(100);
//         std::memset(holder2.data(), 99, holder2.size());
//
//         XFORMAT_V xFormatV2 = {kAvPixFmtUyvy422, 1920, 1080};
//         auto      idx       = xdata::Set(data_sp.get(), 0, xFormatV2, holder2);
//         EXPECT_EQ(idx, 0);
//
//         auto [fmt_v_p, hld_p] = XDataGetWithHolder<xsdk::XFORMAT_V, std::vector<uint8_t>>(data_sp);
//         ASSERT_TRUE(fmt_v_p);
//         EXPECT_TRUE(fmt_v_p->IsEqual(xFormatV2, false));
//         ASSERT_TRUE(hld_p);
//         EXPECT_EQ(hld_p->Size(), 100);
//         EXPECT_EQ(hld_p->At(17), 99);
//     }
//
// }

// NOLINTEND(*)