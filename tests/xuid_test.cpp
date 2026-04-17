#include "xbase.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <thread>

using namespace xsdk;

struct MyStruct {};

template <class TBase>
class TestClass: public std::string {};

using MyUsing = TestClass<int>;
typedef TestClass<int> MyTypedef;

namespace xtest {
class TestClassNS {};

enum MyEnum { kValue = 11, kNext };

enum class MyEnumClass { kRed = 11, kBlue, kGreen };

} // namespace xtest

TEST(xuid_tests, type_uid)
{
    constexpr auto uid  = xbase::TypeUid<std::string>();
    constexpr auto uid1 = xbase::TypeUid<std::wstring>();
    constexpr auto uid2 = xbase::TypeUid<TestClass<int>>();
    constexpr auto uid3 = xbase::TypeUid<MyUsing>();
    constexpr auto uid4 = xbase::TypeUid<MyTypedef>();
    EXPECT_TRUE(uid);
    EXPECT_NE(uid1, uid2) << "xbase::TypeUid SAME for std::string & std::wstring";
    EXPECT_EQ(uid3, uid2) << "xbase::TypeUid WRONG for using";
    EXPECT_EQ(uid3, uid4) << "xbase::TypeUid WRONG for typedef";
}

TEST(xuid_tests, type_name)
{
    using namespace xtest;

    constexpr auto name  = xbase::TypeName<std::string>();
    constexpr auto name1 = xbase::TypeName<TestClass<int>>();
    constexpr auto name2 = xbase::TypeName<MyUsing>();
    constexpr auto name3 = xbase::TypeName<MyStruct>();
    constexpr auto name4 = xbase::TypeName<TestClassNS>();
    constexpr auto name5 = xbase::TypeName<MyTypedef>();
    constexpr auto name6 = xbase::TypeName<MyEnum>();
    constexpr auto name7 = xbase::TypeName<MyEnumClass>();
    std::cout << name << std::endl;
    std::cout << name1 << std::endl;
    std::cout << name2 << std::endl;
    std::cout << name3 << std::endl;
    std::cout << name4 << std::endl;
    std::cout << name5 << std::endl;
    std::cout << name6 << std::endl;
    std::cout << name7 << std::endl;
    EXPECT_EQ(name1, name2);
    EXPECT_EQ(name1, name5);

    EXPECT_EQ(name1, "TestClass<int>");
    EXPECT_EQ(name2, "TestClass<int>");
    EXPECT_EQ(name3, "MyStruct");
    EXPECT_EQ(name4, "xtest::TestClassNS");
    EXPECT_EQ(name5, "TestClass<int>");
    EXPECT_EQ(name6, "xtest::MyEnum");
    EXPECT_EQ(name7, "xtest::MyEnumClass");
}

TEST(xuid_tests, make_uid_from_invalid_uid)
{
    EXPECT_NE(xbase::MakeUid(xbase::kInvalidUid), xbase::MakeUid(xbase::kInvalidUid));
    EXPECT_NE(xbase::MakeUid(xbase::kInvalidUid, xbase::kInvalidUid),
              xbase::MakeUid(xbase::kInvalidUid, xbase::kInvalidUid));

    EXPECT_EQ(xbase::MakeUid(xbase::kInvalidUid, 123), xbase::MakeUid(xbase::kInvalidUid, 123));

    EXPECT_EQ(xbase::MakeUid(321, xbase::kInvalidUid), xbase::MakeUid(321, xbase::kInvalidUid));
}

TEST(xuid_tests, make_uid)
{
    auto uid1 = xbase::MakeUid(10);
    EXPECT_EQ(uid1, xbase::MakeUid(10));
    EXPECT_EQ(uid1, xbase::MakeUid(10));
    EXPECT_NE(xbase::MakeUid(10), xbase::MakeUid(12));

    auto uid2 = xbase::MakeUid(10, 11);
    EXPECT_EQ(uid2, xbase::MakeUid(10, 11));
    EXPECT_EQ(uid2, xbase::MakeUid(10, 11));
    EXPECT_NE(xbase::MakeUid(13, 11), xbase::MakeUid(10, 11));
    EXPECT_NE(xbase::MakeUid(10, 16), xbase::MakeUid(10, 11));
    EXPECT_NE(xbase::MakeUid(15, 16), xbase::MakeUid(10, 11));

    auto uid3 = xbase::MakeUid("some string");
    EXPECT_EQ(uid3, xbase::MakeUid("some string"));
    EXPECT_EQ(uid3, xbase::MakeUid("some string"));
    EXPECT_NE(xbase::MakeUid("some other string"), xbase::MakeUid("some string"));

    EXPECT_EQ(uid1, xbase::MakeUid(10));
    EXPECT_EQ(uid2, xbase::MakeUid(10, 11));
    EXPECT_EQ(uid3, xbase::MakeUid("some string"));
    EXPECT_NE(uid1, uid2);
    EXPECT_NE(uid1, uid3);
    EXPECT_NE(uid2, uid3);
}

TEST(xuid_tests, make_uid_in_multi_threads)
{
    size_t                   num_threads = 100;
    std::vector<std::thread> threads;
    std::vector<uint64_t>    uids1(num_threads, 0);
    std::vector<uint64_t>    uids2(num_threads, 0);
    std::vector<uint64_t>    uids3(num_threads, 0);

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&, i = i]() {
            uids1[i] = xbase::MakeUid(123);
            uids2[i] = xbase::MakeUid(123, 321);
            uids3[i] = xbase::MakeUid("other string");
        });
    }
    std::for_each(threads.begin(), threads.end(), [](auto& th) { th.join(); });

    auto uid1 = xbase::MakeUid(123);
    auto uid2 = xbase::MakeUid(123, 321);
    auto uid3 = xbase::MakeUid("other string");
    for (int i = 0; i < num_threads; i++) {
        EXPECT_EQ(uid1, uids1[i]);
        EXPECT_EQ(uid2, uids2[i]);
        EXPECT_EQ(uid3, uids3[i]);
    }
}

TEST(xuid_tests, hash_string_test)
{
    std::vector<std::string> parts = {"FirstPart", "NextPart", "A", "_", "XXX", "", "::", ":", "LAST"};

    std::string               accum;
    std::optional<xbase::Uid> hash_seq;

    std::set<xbase::Uid> values;
    for (const auto& s : parts) {
        accum += s;
        auto hash_direct = xbase::HashString(accum);
        hash_seq         = xbase::HashString(s, hash_seq);

        EXPECT_EQ(hash_direct, hash_seq.value());

        auto new_one = values.emplace(hash_seq.value()).second;
        EXPECT_EQ(new_one, !s.empty());
    }
}
