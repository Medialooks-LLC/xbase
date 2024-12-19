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

TEST(xdata_tests, data_set_to_null)
{
    IData* data_p = nullptr;

    std::string str = "my test";
    auto        idx = xdata::Set(data_p, -1, str);
    EXPECT_EQ(idx, -1);

    idx = xdata::Set(data_p, -1, str, std::move(str));
    EXPECT_EQ(idx, -1);
}

TEST(xdata_tests, data_count_on_null)
{
    IData* data_p = nullptr;

    auto idx = xdata::Count<std::string>(data_p);
    EXPECT_EQ(idx, 0);
}

TEST(xdata_tests, data_get_from_null)
{
    IData* data_p = nullptr;

    auto str_sp = xdata::Get<std::string>(data_p);
    ASSERT_FALSE(str_sp);
}

TEST(xdata_tests, data_get_non_exists)
{
    auto data_sp = xdata::Create();

    std::string str = "my test";
    auto        idx = xdata::Set(data_sp.get(), -1, str);
    EXPECT_EQ(idx, 0);

    auto int_sp = xdata::Get<int64_t>(data_sp.get());
    ASSERT_FALSE(int_sp);
}

TEST(xdata_tests, data_get_with_holder)
{
    auto data_sp = xdata::Create();
    struct MyStruct {
        std::string str = "my test";
    } my_struct;

    auto idx = xdata::Set(data_sp.get(), -1, my_struct.str, std::move(&my_struct));
    EXPECT_EQ(idx, 0);

    std::any any_holder;
    auto     str_sp = xdata::Get<std::string>(data_sp.get(), 0, &any_holder);
    ASSERT_TRUE(str_sp);
    EXPECT_STREQ(str_sp->c_str(), "my test") << "Returned wrong data: '" << *str_sp << "'";
    auto res = xdata::AnyUnwrap<MyStruct*>(any_holder);
    ASSERT_TRUE(res);

    EXPECT_STREQ((*res)->str.c_str(), "my test") << "Returned wrong holder: '" << (*res)->str << "'";
}

TEST(xdata_tests, data_types_count)
{
    auto data_sp = xdata::Create();

    std::string str_val = "my test";
    auto        idx     = xdata::Set(data_sp.get(), -1, str_val);
    EXPECT_EQ(idx, 0);
    int64_t int_val = 123;
    idx             = xdata::Set(data_sp.get(), -1, int_val);
    EXPECT_EQ(idx, 0);
    int64_t int_val2 = 321;
    idx              = xdata::Set(data_sp.get(), -1, int_val2);
    EXPECT_EQ(idx, 1);
    double double_val = 1.23;
    idx               = xdata::Set(data_sp.get(), -1, double_val);
    EXPECT_EQ(idx, 0);

    EXPECT_EQ(data_sp->TypesCount(), 3) << "Wrong count of stored data types";
}

TEST(xdata_tests, data_remove)
{
    auto data_sp = xdata::Create();

    int64_t int_val = 123;
    auto    idx     = xdata::Set(data_sp.get(), -1, int_val);
    EXPECT_EQ(idx, 0);
    data_sp->DataRemove(xbase::TypeUid<int64_t>(), 0);
    int64_t int_val2 = 321;
    idx              = xdata::Set(data_sp.get(), -1, int_val2);
    EXPECT_EQ(idx, 0);
}

TEST(xdata_tests, data_remove_non_exists)
{
    auto data_sp = xdata::Create();

    int64_t int_val = 123;
    auto    idx     = xdata::Set(data_sp.get(), -1, int_val);
    EXPECT_EQ(idx, 0);
    auto res = data_sp->DataRemove(xbase::TypeUid<int64_t>(), 2);

    EXPECT_FALSE(res.first.has_value());
    EXPECT_FALSE(res.second.has_value());
}

TEST(xdata_tests, data_reset)
{
    auto data_sp = xdata::Create();

    std::string str_val = "my test";
    auto        idx     = xdata::Set(data_sp.get(), -1, str_val);
    EXPECT_EQ(idx, 0);
    int64_t int_val = 123;
    idx             = xdata::Set(data_sp.get(), -1, int_val);
    EXPECT_EQ(idx, 0);
    data_sp->DataReset(xbase::TypeUid<int64_t>());
    EXPECT_EQ(data_sp->TypesCount(), 1) << "Wrong count of stored data types";
    int64_t int_val2 = 321;
    idx              = xdata::Set(data_sp.get(), -1, int_val2);
    EXPECT_EQ(idx, 0);
}

TEST(xdata_tests, data_copty_to_non_exist_dest)
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

    auto res = data_sp->CopyTo(nullptr, false);

    EXPECT_EQ(res, 0) << "Something copied to non exists destination";
}

TEST(xdata_tests, data_copy_to)
{
    auto data_sp      = xdata::Create();
    auto dest_data_sp = xdata::Create();

    std::string str_val = "my test";
    auto        idx     = xdata::Set(data_sp.get(), -1, str_val);
    EXPECT_EQ(idx, 0);
    int64_t int_val = 123;
    idx             = xdata::Set(data_sp.get(), -1, int_val);
    EXPECT_EQ(idx, 0);
    double double_val = 1.23;
    idx               = xdata::Set(data_sp.get(), -1, double_val);
    EXPECT_EQ(idx, 0);
    double double_val2 = 3.21;
    idx                = xdata::Set(dest_data_sp.get(), -1, double_val2);
    EXPECT_EQ(idx, 0);

    EXPECT_EQ(xdata::Count<std::string>(data_sp.get()), 1);
    EXPECT_EQ(xdata::Count<int64_t>(data_sp.get()), 1);
    EXPECT_EQ(xdata::Count<double>(data_sp.get()), 1);

    auto res = data_sp->CopyTo(dest_data_sp.get(), true);
    EXPECT_EQ(res, 3);

    EXPECT_EQ(xdata::Count<std::string>(dest_data_sp.get()), 1);
    EXPECT_EQ(xdata::Count<int64_t>(dest_data_sp.get()), 1);
    EXPECT_EQ(xdata::Count<double>(dest_data_sp.get()), 1);
}

TEST(xdata_tests, data_copy_to_with_types)
{
    auto data_sp      = xdata::Create();
    auto dest_data_sp = xdata::Create();

    std::string str_val = "my test";
    auto        idx     = xdata::Set(data_sp.get(), -1, str_val);
    EXPECT_EQ(idx, 0);
    int64_t int_val = 123;
    idx             = xdata::Set(data_sp.get(), -1, int_val);
    EXPECT_EQ(idx, 0);
    double double_val = 1.23;
    idx               = xdata::Set(data_sp.get(), -1, double_val);
    EXPECT_EQ(idx, 0);
    double double_val2 = 3.21;
    idx                = xdata::Set(dest_data_sp.get(), -1, double_val2);
    EXPECT_EQ(idx, 0);

    EXPECT_EQ(xdata::Count<std::string>(data_sp.get()), 1);
    EXPECT_EQ(xdata::Count<int64_t>(data_sp.get()), 1);
    EXPECT_EQ(xdata::Count<double>(data_sp.get()), 1);

    auto res = data_sp->CopyTo(dest_data_sp.get(), false, {xbase::TypeUid<int64_t>(), xbase::TypeUid<char>()});
    EXPECT_EQ(res, 1);

    EXPECT_EQ(xdata::Count<std::string>(dest_data_sp.get()), 1);
    EXPECT_EQ(xdata::Count<int64_t>(dest_data_sp.get()), 0);
    EXPECT_EQ(xdata::Count<double>(dest_data_sp.get()), 1);
}

TEST(xdata_tests, data_copy_to_include)
{
    auto data_sp      = xdata::Create();
    auto dest_data_sp = xdata::Create();

    std::string str_val = "my test";
    auto        idx     = xdata::Set(data_sp.get(), -1, str_val);
    EXPECT_EQ(idx, 0);
    int64_t int_val = 123;
    idx             = xdata::Set(data_sp.get(), -1, int_val);
    EXPECT_EQ(idx, 0);
    double double_val = 1.23;
    idx               = xdata::Set(data_sp.get(), -1, double_val);
    EXPECT_EQ(idx, 0);

    double double_val2 = 3.21;
    idx                = xdata::Set(dest_data_sp.get(), -1, double_val2);
    EXPECT_EQ(idx, 0);

    EXPECT_EQ(xdata::Count<std::string>(data_sp.get()), 1);
    EXPECT_EQ(xdata::Count<int64_t>(data_sp.get()), 1);
    EXPECT_EQ(xdata::Count<double>(data_sp.get()), 1);

    auto res = data_sp->CopyTo(dest_data_sp.get(),
                               false,
                               {xbase::TypeUid<int64_t>(), xbase::TypeUid<double>(), xbase::TypeUid<char>()},
                               IData::CloneSetType::Include);
    EXPECT_EQ(res, 1);

    EXPECT_EQ(xdata::Count<std::string>(dest_data_sp.get()), 0);
    EXPECT_EQ(xdata::Count<int64_t>(dest_data_sp.get()), 1);
    EXPECT_EQ(xdata::Count<double>(dest_data_sp.get()), 1);
    EXPECT_EQ(*xdata::Get<double>(dest_data_sp.get(), 0), 3.21);
}

TEST(xdata_tests, data_clone_exclude)
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
    auto               clone_sp = data_sp->Clone(types, IData::CloneSetType::Exclude);

    EXPECT_EQ(xdata::Count<std::string>(clone_sp.get()), 1);
    EXPECT_EQ(xdata::Count<int64_t>(clone_sp.get()), 0);
    EXPECT_EQ(xdata::Count<double>(clone_sp.get()), 0);
}

TEST(xdata_tests, data_clone_exclude_with_empty_cloned_types)
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

    std::set<uint64_t> types {};
    auto               clone_sp = data_sp->Clone(types, IData::CloneSetType::Exclude);

    EXPECT_EQ(xdata::Count<std::string>(clone_sp.get()), 1);
    EXPECT_EQ(xdata::Count<int64_t>(clone_sp.get()), 1);
    EXPECT_EQ(xdata::Count<double>(clone_sp.get()), 1);
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