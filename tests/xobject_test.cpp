#include "xbase.h"

#include <gtest/gtest.h>

using namespace xsdk;

// NOLINTBEGIN(*)
class IObjectTest final: public IObject, public std::enable_shared_from_this<IObjectTest> {

    const uint64_t uid_;
    std::string    name_;

    IObjectTest(std::string_view _name) : uid_(xbase::NextUid()), name_(_name) {}
public:
    static std::shared_ptr<IObjectTest> Create(std::string_view _name = {})
    {
        return std::shared_ptr<IObjectTest> {new IObjectTest(_name)};
    }

    uint64_t ObjectUid() const override { return uid_; };

    std::any QueryPtr(xbase::Uid _type_query) override
    {
        try {
            if (_type_query == xbase::TypeUid<IObjectTest>())
                    return std::static_pointer_cast<IObjectTest>(shared_from_this());

            if (_type_query == xbase::TypeUid<IObject>())
                return std::static_pointer_cast<IObject>(shared_from_this());
        }
        catch (std::bad_weak_ptr const&) { //If we don't have any shared pointers before call this method
            return {};
        }
        return {};
    };
    std::any QueryPtrC(xbase::Uid _type_query) const override
    {
        try {
            if (_type_query == xbase::TypeUid<const IObjectTest>())
                    return std::static_pointer_cast<const IObjectTest>(shared_from_this());

            if (_type_query == xbase::TypeUid<const IObject>())
                return std::static_pointer_cast<const IObject>(shared_from_this());

        }
        catch (std::bad_weak_ptr const&) {
            return {};
        }

        return {};
    };

    void             NameSet(std::string_view _name) { name_ = _name; };
    std::string_view NameGet() const { return name_; };
};

TEST(xobject_test, query_ptr_invalid_type)
{

    auto io_test = IObjectTest::Create();

    auto null_any_ptr = io_test->QueryPtr(xbase::TypeUid<int>());
    EXPECT_FALSE(null_any_ptr.has_value());
}

TEST(xobject_test, query_ptr_c_invalid_type)
{

    auto io_test = IObjectTest::Create();

    auto null_any_ptr = io_test->QueryPtrC(xbase::TypeUid<int>());
    EXPECT_FALSE(null_any_ptr.has_value());
}

TEST(xobject_test, query_ptr_direct_call)
{
    auto io_test = IObjectTest::Create();

    auto io_test_any_ptr = io_test->QueryPtr(xbase::TypeUid<IObjectTest>());
    EXPECT_TRUE(io_test_any_ptr.has_value());
}

TEST(xobject_test, query_ptr_c_direct_call)
{
    auto io_test = IObjectTest::Create();

    auto io_test_any_ptr = io_test->QueryPtrC(xbase::TypeUid<const IObjectTest>());
    EXPECT_TRUE(io_test_any_ptr.has_value());
}

TEST(xobject_test, query_ptr_c_direct_call_on_non_const)
{
    auto io_test = IObjectTest::Create();

    auto io_test_any_ptr = io_test->QueryPtrC(xbase::TypeUid<const IObjectTest>());
    EXPECT_TRUE(io_test_any_ptr.has_value());
}

TEST(xobject_test, ptr_query)
{
    auto io_test = IObjectTest::Create();
    io_test->NameSet("some name");

    auto obj_qp_sp = xobject::PtrQuery<IObject>(io_test.get());
    EXPECT_TRUE(obj_qp_sp);
    auto obj_sp = std::static_pointer_cast<IObject>(io_test);
    EXPECT_EQ(obj_qp_sp, obj_sp);

    auto obj_sp2 = xobject::PtrQuery<IObjectTest>(obj_sp.get());
    EXPECT_EQ("some name", obj_sp2->NameGet());
    obj_sp2->NameSet("other name");
    EXPECT_EQ("other name", obj_sp2->NameGet());
    EXPECT_EQ("other name", io_test->NameGet());
}

TEST(xobject_test, ptr_query_const_with_non_const)
{
    auto io_test = IObjectTest::Create();
    io_test->NameSet("some name");

    const IObject* const_p   = io_test.get();
    auto obj_qp_sp = xobject::PtrQuery<const IObject>(const_p);
    EXPECT_TRUE(obj_qp_sp);
    auto obj_sp = std::static_pointer_cast<const IObject>(io_test);
    EXPECT_EQ(obj_qp_sp, obj_sp);

    auto obj_sp2 = xobject::PtrQuery<const IObjectTest>(obj_sp.get());
    EXPECT_EQ("some name", obj_sp2->NameGet());
    io_test->NameSet("other name");
    EXPECT_EQ("other name", obj_sp2->NameGet());
}

TEST(xobject_test, ptr_query_const)
{

    std::shared_ptr<const IObjectTest> io_test = IObjectTest::Create("some name");

    auto obj_qp_sp = xobject::PtrQuery<const IObject>(io_test.get());
    EXPECT_TRUE(obj_qp_sp);
    auto obj_sp = std::static_pointer_cast<const IObject>(io_test);
    EXPECT_EQ(obj_qp_sp, obj_sp);

    auto obj_sp2 = xobject::PtrQuery<const IObjectTest>(obj_sp.get());
    EXPECT_EQ("some name", obj_sp2->NameGet());
}

// NOLINTEND(*)