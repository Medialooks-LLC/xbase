#include "xbase.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <thread>

// TEMP
// #include "../../include/xmodules/Struct_Media.h"

using namespace xsdk;

// NOLINTBEGIN(*)

class Test1 {
public:
    std::string value;

    Test1(const std::string& _val) : value(_val) {}
};

class Test2: public Test1 {
public:
    std::string value2;

    Test2(const Test1& _t1) : Test1(_t1), value2(_t1.value + "_outer") {}
    Test2(const std::string& _val) : Test1(_val + "_inner"), value2(_val) {}
};

class Test3 {
public:
    std::string value3;

    Test3(const std::string& _val) : value3(_val) {}
    Test3(const Test1& _t1) : value3(_t1.value) {}
};

TEST(holder_tests, conversion_test)
{
    xbase::HolderP<Test1> holder_1(Test1("t1"), {});
    xbase::HolderP<Test2> holder_2(Test2("t2"), {});
    xbase::HolderP<Test3> holder_3(Test3("t3"), {});
    ASSERT_EQ(holder_1->value, "t1");
    ASSERT_EQ(holder_2->value2, "t2");
    ASSERT_EQ(holder_3->value3, "t3");

    holder_1 = holder_2;
    holder_2 = holder_1;
    holder_3 = holder_2;

    EXPECT_EQ(holder_1->value, "t2_inner");
    EXPECT_EQ(holder_2->value, "t2_inner");
    EXPECT_EQ(holder_2->value2, "t2_inner_outer");
    EXPECT_EQ(holder_3->value3, "t2_inner");

    holder_3 = holder_1;
    EXPECT_EQ(holder_3->value3, "t2_inner");
}

TEST(xbuffer_tests, conversion_test)
{
    auto buffer   = xbase::BufferCreate<uint8_t>(100);
    auto buffer_c = xbase::BufferCreateC<uint8_t>(200);

    buffer_c = buffer;
    ASSERT_EQ(buffer_c->data, buffer->data);
    ASSERT_EQ(buffer_c->size, buffer->size);
    ASSERT_NE((void*)buffer_c.DataPtr(), (void*)buffer.DataPtr());

    xbase::XBufferC buffer_c2(buffer);
    ASSERT_EQ(buffer_c2->data, buffer->data);
    ASSERT_EQ(buffer_c2->size, buffer->size);
    ASSERT_NE((void*)buffer_c2.DataPtr(), (void*)buffer.DataPtr());
}

// NOLINTEND(*)
