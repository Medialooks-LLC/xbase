#include "xbase.h"

#include <gtest/gtest.h>

namespace xsdk::xbase::object {
// NOLINTBEGIN(*)

class INotRegistered {
public:
    virtual ~INotRegistered() = default;

    virtual void Unused() = 0;
};

class ObjectTest final: public IObject, public std::enable_shared_from_this<ObjectTest> {

    const uint64_t uid_;
    std::string    name_;

    explicit ObjectTest(std::string_view _name) : uid_(xbase::NextUid()), name_(_name) {}

public:
    static std::shared_ptr<ObjectTest> Create(std::string_view _name = {})
    {
        return std::shared_ptr<ObjectTest> {new ObjectTest(_name)};
    }

    uint64_t ObjectUid() const override { return uid_; };

    std::any QueryPtr(xbase::Uid _type_query) override
    {
        try {
            if (_type_query == xbase::TypeUid<ObjectTest>())
                return std::static_pointer_cast<ObjectTest>(shared_from_this());

            if (_type_query == xbase::TypeUid<IObject>())
                return std::static_pointer_cast<IObject>(shared_from_this());
        }
        catch (std::bad_weak_ptr const&) { // If we don't have any shared pointers before call this method
            return {};
        }
        return {};
    };

    std::any QueryPtrC(xbase::Uid _type_query) const override
    {
        try {
            if (_type_query == xbase::TypeUid<const ObjectTest>())
                return std::static_pointer_cast<const ObjectTest>(shared_from_this());

            if (_type_query == xbase::TypeUid<const IObject>())
                return std::static_pointer_cast<const IObject>(shared_from_this());
        }
        catch (std::bad_weak_ptr const&) {
            return {};
        }

        return {};
    };

    void NameSet(std::string_view _name) { name_ = _name; };

    std::string_view NameGet() const { return name_; };
};

class IDerived: public IObject {
public:
    virtual void             NameSet(std::string_view _name) = 0;
    virtual std::string_view NameGet() const                 = 0;
};

class IDerived2: public IDerived {
public:
    virtual void             NameSet2(std::string_view _name) = 0;
    virtual std::string_view NameGet2() const                 = 0;
};

class IExtra {
public:
    virtual ~IExtra() = default;

    virtual void             NameExtraSet(std::string_view _name) = 0;
    virtual std::string_view NameExtraGet() const                 = 0;
};

class Derived2: public xbase::ObjectBase<Derived2, IDerived2, IDerived, IExtra>, public IExtra {
    std::string name_;
    std::string name2_;
    std::string name_ex_;

public:
    using IObjectImpl_ = xbase::ObjectBase<Derived2, IDerived2, IDerived, IExtra>;

public:
    explicit Derived2(const xbase::Uid _obj_uid) : IObjectImpl_(_obj_uid) {}

    void NameSet(std::string_view _name) override { name_ = _name; };

    std::string_view NameGet() const override { return name_; };

    void NameSet2(std::string_view _name) override { name2_ = _name; };

    std::string_view NameGet2() const override { return name2_; };

    void NameExtraSet(std::string_view _name) override { name_ex_ = _name; };

    std::string_view NameExtraGet() const override { return name_ex_; };
};

TEST(xobject_test, derived_test)
{
    IObject::SPtr derived_obj = std::make_shared<Derived2>(123);
    ASSERT_TRUE(derived_obj);

    EXPECT_EQ(derived_obj->ObjectUid(), 123);

    auto derived = xobject::PtrQuery<IDerived>(derived_obj.get());
    ASSERT_TRUE(derived);
    derived->NameSet("123");
    EXPECT_EQ(derived->NameGet(), "123");
    auto derived_2 = xobject::PtrQuery<IDerived2>(derived_obj.get());
    ASSERT_TRUE(derived_2);
    derived_2->NameSet2("567");
    EXPECT_EQ(derived_2->NameGet2(), "567");
    auto derived_ex = xobject::PtrQuery<IExtra>(derived_obj.get());
    ASSERT_TRUE(derived_ex);
    derived_ex->NameExtraSet("extra");
    EXPECT_EQ(derived_ex->NameExtraGet(), "extra");
}

TEST(xobject_test, query_ptr_invalid_type)
{

    auto io_test = ObjectTest::Create();

    auto null_any_ptr = io_test->QueryPtr(xbase::TypeUid<int>());
    EXPECT_FALSE(null_any_ptr.has_value());
}

TEST(xobject_test, query_ptr_c_invalid_type)
{

    auto io_test = ObjectTest::Create();

    auto null_any_ptr = io_test->QueryPtrC(xbase::TypeUid<int>());
    EXPECT_FALSE(null_any_ptr.has_value());
}

TEST(xobject_test, query_ptr_direct_call)
{
    auto io_test = ObjectTest::Create();

    auto io_test_any_ptr = io_test->QueryPtr(xbase::TypeUid<ObjectTest>());
    EXPECT_TRUE(io_test_any_ptr.has_value());
}

TEST(xobject_test, query_ptr_c_direct_call)
{
    auto io_test = ObjectTest::Create();

    auto io_test_any_ptr = io_test->QueryPtrC(xbase::TypeUid<const ObjectTest>());
    EXPECT_TRUE(io_test_any_ptr.has_value());
}

TEST(xobject_test, query_ptr_c_direct_call_on_non_const)
{
    auto io_test = ObjectTest::Create();

    auto io_test_any_ptr = io_test->QueryPtrC(xbase::TypeUid<const ObjectTest>());
    EXPECT_TRUE(io_test_any_ptr.has_value());
}

TEST(xobject_test, ptr_query)
{
    auto io_test = ObjectTest::Create();
    io_test->NameSet("some name");

    auto obj_qp_sp = xobject::PtrQuery<IObject>(io_test.get());
    EXPECT_TRUE(obj_qp_sp);
    auto obj_sp = std::static_pointer_cast<IObject>(io_test);
    EXPECT_EQ(obj_qp_sp, obj_sp);

    auto obj_sp2 = xobject::PtrQuery<ObjectTest>(obj_sp.get());
    EXPECT_EQ("some name", obj_sp2->NameGet());
    obj_sp2->NameSet("other name");
    EXPECT_EQ("other name", obj_sp2->NameGet());
    EXPECT_EQ("other name", io_test->NameGet());
}

TEST(xobject_test, ptr_query_const_with_non_const)
{
    auto io_test = ObjectTest::Create();
    io_test->NameSet("some name");

    const IObject* const_p   = io_test.get();
    auto           obj_qp_sp = xobject::PtrQuery<const IObject>(const_p);
    EXPECT_TRUE(obj_qp_sp);
    auto obj_sp = std::static_pointer_cast<const IObject>(io_test);
    EXPECT_EQ(obj_qp_sp, obj_sp);

    auto obj_sp2 = xobject::PtrQuery<const ObjectTest>(obj_sp.get());
    EXPECT_EQ("some name", obj_sp2->NameGet());
    io_test->NameSet("other name");
    EXPECT_EQ("other name", obj_sp2->NameGet());
}

TEST(xobject_test, ptr_query_const)
{

    std::shared_ptr<const ObjectTest> io_test = ObjectTest::Create("some name");

    auto obj_qp_sp = xobject::PtrQuery<const IObject>(io_test.get());
    EXPECT_TRUE(obj_qp_sp);
    auto obj_sp = std::static_pointer_cast<const IObject>(io_test);
    EXPECT_EQ(obj_qp_sp, obj_sp);

    auto obj_sp2 = xobject::PtrQuery<const ObjectTest>(obj_sp.get());
    EXPECT_EQ("some name", obj_sp2->NameGet());
}

TEST(xobject_test, ptr_query_from_null)
{
    std::shared_ptr<ObjectTest> io_test;

    auto obj_qp_sp = xobject::PtrQuery<IObject>(io_test.get());
    EXPECT_FALSE(obj_qp_sp);
}

TEST(xobject_test, ptr_query_const_from_null)
{
    std::shared_ptr<const ObjectTest> io_test;

    auto obj_qp_sp = xobject::PtrQuery<const IObject>(io_test.get());
    EXPECT_FALSE(obj_qp_sp);
}

TEST(xobject_test, object_base_query_ptr_unknown_type_returns_empty_any)
{
    IObject::SPtr derived_obj = std::make_shared<Derived2>(123);
    ASSERT_TRUE(derived_obj);

    auto queried = derived_obj->QueryPtr(xbase::TypeUid<INotRegistered>());
    EXPECT_FALSE(queried.has_value());
}

TEST(xobject_test, object_base_query_ptrc_unknown_type_returns_empty_any)
{
    const IObject::SPtr derived_obj = std::make_shared<Derived2>(123);
    ASSERT_TRUE(derived_obj);

    auto queried = derived_obj->QueryPtrC(xbase::TypeUid<const INotRegistered>());
    EXPECT_FALSE(queried.has_value());
}

TEST(xobject_test, object_base_ptr_query_unknown_type_returns_nullptr)
{
    IObject::SPtr derived_obj = std::make_shared<Derived2>(123);
    ASSERT_TRUE(derived_obj);

    auto queried = xobject::PtrQuery<INotRegistered>(derived_obj.get());
    EXPECT_EQ(queried, nullptr);
}

TEST(xobject_test, object_base_ptr_query_const_unknown_type_returns_nullptr)
{
    const IObject::SPtr derived_obj = std::make_shared<Derived2>(123);
    ASSERT_TRUE(derived_obj);

    auto queried = xobject::PtrQuery<INotRegistered>(derived_obj.get());
    EXPECT_EQ(queried, nullptr);
}

TEST(xobject_test, object_base_query_ptrc_requires_const_type_uid)
{
    const IObject::SPtr derived_obj = std::make_shared<Derived2>(123);
    ASSERT_TRUE(derived_obj);

    auto queried = derived_obj->QueryPtrC(xbase::TypeUid<IExtra>());
    EXPECT_FALSE(queried.has_value());
}

TEST(xobject_test, object_base_ptr_query_null_object_returns_nullptr)
{
    EXPECT_EQ(xobject::PtrQuery<IDerived>(static_cast<IObject*>(nullptr)), nullptr);
    EXPECT_EQ(xobject::PtrQuery<IDerived>(static_cast<const IObject*>(nullptr)), nullptr);
}

TEST(xobject_test, object_base_query_ptr_before_shared_ptr_returns_empty_any)
{
    Derived2 derived_obj(321);

    auto queried = derived_obj.QueryPtr(xbase::TypeUid<IDerived2>());
    EXPECT_FALSE(queried.has_value());
}

TEST(xobject_test, object_base_query_ptrc_before_shared_ptr_returns_empty_any)
{
    const Derived2 derived_obj(321);

    auto queried = derived_obj.QueryPtrC(xbase::TypeUid<const IDerived2>());
    EXPECT_FALSE(queried.has_value());
}

} // namespace xsdk::xbase::object

// NOLINTEND(*)