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

TEST(xdata_tests, interfaces_test)
{
    struct MyTestStruct {
        std::string name;
        double      val;
    };

    struct MyTestStruct2: public MyTestStruct {};
    struct MyTestStruct3: public MyTestStruct {};
    struct MyTestStruct4: public MyTestStruct {};

    IData::UPtr data_p;
    {
        auto                       string_sp1 = std::make_shared<std::string>("str_1");
        auto                       string_sp2 = std::make_shared<std::string>("str_2");
        std::weak_ptr<std::string> string_wp1 = string_sp1;
        std::weak_ptr<std::string> string_wp2 = string_sp2;

        auto struct_sp  = std::make_shared<MyTestStruct>(MyTestStruct {"MyTestStruct", 17.0});
        auto struct2_sp = std::make_shared<MyTestStruct2>(MyTestStruct2 {"MyTestStruct2", 11.0});
        auto struct3_sp = std::make_shared<MyTestStruct3>(MyTestStruct3 {"MyTestStruct3", 23.0});
        std::weak_ptr<MyTestStruct2> struct2_wp = struct2_sp;
        std::weak_ptr<MyTestStruct3> struct3_wp = struct3_sp;

        data_p = xdata::CreateInterfacesCollection(string_sp1,
                                                   string_wp1,
                                                   string_wp2,
                                                   struct_sp,
                                                   struct_sp,
                                                   struct2_wp,
                                                   struct3_sp,
                                                   struct3_wp);
    }

    ASSERT_TRUE(data_p);
    EXPECT_EQ(xdata::Count<std::string>(data_p.get()), 3);
    EXPECT_EQ(xdata::Count<MyTestStruct>(data_p.get()), 2);
    EXPECT_EQ(xdata::Count<MyTestStruct2>(data_p.get()), 1);
    EXPECT_EQ(xdata::Count<MyTestStruct3>(data_p.get()), 2);
    EXPECT_EQ(xdata::Count<MyTestStruct4>(data_p.get()), 0);

    auto shared_str_vec = xdata::GetSharedVec<std::string>(data_p.get());
    ASSERT_EQ(shared_str_vec.size(), 2);
    ASSERT_TRUE(shared_str_vec[0]);
    ASSERT_TRUE(shared_str_vec[1]);
    EXPECT_EQ(*shared_str_vec[0], "str_1");
    EXPECT_EQ(*shared_str_vec[1], "str_1");

    auto struct_sp = xdata::GetShared<MyTestStruct>(data_p.get());
    ASSERT_TRUE(struct_sp);
    EXPECT_EQ(struct_sp->name, "MyTestStruct");
    auto struct2_sp = xdata::GetShared<MyTestStruct2>(data_p.get());
    ASSERT_FALSE(struct2_sp);
    auto struct3_sp = xdata::GetShared<MyTestStruct3>(data_p.get());
    ASSERT_TRUE(struct3_sp);
    EXPECT_EQ(struct3_sp->name, "MyTestStruct3");

    auto struct4_sp = xdata::GetShared<MyTestStruct4>(data_p.get());
    EXPECT_FALSE(struct4_sp);

    auto shared_vec = xdata::GetSharedVec<MyTestStruct>(data_p.get());
    ASSERT_EQ(shared_vec.size(), 2);
    ASSERT_TRUE(shared_vec[0]);
    ASSERT_TRUE(shared_vec[1]);
    EXPECT_EQ(shared_vec[0]->name, "MyTestStruct");
    EXPECT_EQ(shared_vec[1]->name, "MyTestStruct");

    auto empty_vec = xdata::GetSharedVec<MyTestStruct4>(data_p.get());
    EXPECT_TRUE(empty_vec.empty());
}

TEST(xdata_tests, interfaces_test_const)
{
    auto data_p1 = xdata::Create();
    auto data_p2 = xdata::Create();

    std::shared_ptr<const std::string> str_sp_c = std::make_shared<std::string>("1234");
    xdata::SetShared(data_p1.get(), 0, str_sp_c);

    auto check_ok = xdata::GetSharedConst<std::string>(data_p1.get());
    EXPECT_TRUE(check_ok);

    auto check_fail = xdata::GetShared<std::string>(data_p1.get());
    EXPECT_FALSE(check_fail);

    auto str_sp = std::make_shared<std::string>("567");
    xdata::SetShared(data_p2.get(), 0, str_sp);

    check_ok = xdata::GetSharedConst<std::string>(data_p2.get());
    EXPECT_TRUE(check_ok);

    auto check_ok2 = xdata::GetShared<std::string>(data_p2.get());
    EXPECT_TRUE(check_ok2);
}

TEST(xdata_tests, get_vector_test)
{
    auto data_p = xdata::Create();

    xdata::Set(data_p.get(), -1, std::string("123"));
    xdata::Set(data_p.get(), -1, std::string("456"));
    xdata::Set(data_p.get(), -1, std::string("789"));

    EXPECT_EQ(xdata::Count<std::string>(data_p.get()), 3);

    auto str_vec = xdata::GetCopyVec<std::string>(data_p.get());
    ASSERT_EQ(str_vec.size(), 3);
    EXPECT_EQ(str_vec[0], "123");
    EXPECT_EQ(str_vec[1], "456");
    EXPECT_EQ(str_vec[2], "789");

    data_p.reset();
    auto empty_vec = xdata::GetCopyVec<std::string>(data_p.get());
    EXPECT_TRUE(empty_vec.empty());
}

TEST(xdata_tests, get_wrong_index)
{
    auto data_p = xdata::Create();

    xdata::Set(data_p.get(), -1, std::string("123"));
    xdata::Set(data_p.get(), -1, std::string("456"));
    xdata::Set(data_p.get(), -1, std::string("789"));

    auto wrong_1_sp = xdata::Get<std::string>(data_p.get(), 100);
    EXPECT_FALSE(wrong_1_sp);
    auto wrong_2_sp = xdata::Get<std::string>(data_p.get(), xbase::npos);
    EXPECT_FALSE(wrong_2_sp);

    auto wrong_1 = xdata::GetCopy<std::string>(data_p.get(), 100);
    EXPECT_TRUE(wrong_1.empty());
    auto wrong_2 = xdata::GetCopy<std::string>(data_p.get(), xbase::npos);
    EXPECT_TRUE(wrong_2.empty());

    {
        struct WrongHolder {
            size_t fake;
        };

        auto [wrong_1_sp, holder_1_sp] = xdata::GetWithHolder<std::string, WrongHolder>(data_p.get(), 100);
        EXPECT_FALSE(wrong_1_sp);
        EXPECT_FALSE(holder_1_sp);
        auto [wrong_2_sp, holder_2_sp] = xdata::GetWithHolder<std::string, WrongHolder>(data_p.get(), xbase::npos);
        EXPECT_FALSE(wrong_2_sp);
        EXPECT_FALSE(holder_2_sp);
    }
}

TEST(xdata_tests, interfaces_test_fails)
{
    struct MyTestStruct {
        std::string name;
        double      val;
    };

    struct MyTestStruct2: public MyTestStruct {};
    struct MyTestStruct3: public MyTestStruct {};

    IData::UPtr data_p;
    EXPECT_EQ(xdata::Count<std::string>(data_p.get()), 0);
    EXPECT_EQ(xdata::Count<MyTestStruct>(data_p.get()), 0);
    EXPECT_EQ(xdata::Count<MyTestStruct2>(data_p.get()), 0);
    EXPECT_EQ(xdata::Count<MyTestStruct3>(data_p.get()), 0);

    auto shared_str_vec = xdata::GetSharedVec<std::string>(data_p.get());
    ASSERT_EQ(shared_str_vec.size(), 0);

    auto struct_sp = xdata::GetShared<MyTestStruct>(data_p.get());
    EXPECT_FALSE(struct_sp);
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