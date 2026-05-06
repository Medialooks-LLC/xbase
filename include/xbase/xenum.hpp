// (c) 2016 Yakov Litvitskiy <thedsi100@gmail.com>
// LICENSE.txt:
//    Permission is hereby granted, free of charge,
//    to any person obtaining a copy of this software and associated documentation files(the "Software"),
//    to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge,
//    publish, distribute, sublicense, and / or sell copies of the Software,
//    and to permit persons to whom the Software is furnished to do so,
//    subject to the following conditions:
//
//    The above copyright notice and this permission notice shall be included in all copies
//    or substantial portions of the Software.
//
//    THE SOFTWARE IS PROVIDED "AS IS",
//    WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//    FITNESS FOR A PARTICULAR PURPOSE AND
//    NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//    DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// See https://habr.com/ru/post/276763/

#pragma once

#include "xbase/strings.h"

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace xsdk {

//-------------------------------- Public Interface --------------------------------

#define XENUM_OPS32(_enum_name)                                                                               \
    constexpr xenum::Value<_enum_name> operator&(_enum_name l, _enum_name r)                                  \
    {                                                                                                         \
        return _enum_name(uint32_t(l) & uint32_t(r));                                                         \
    }                                                                                                         \
    constexpr xenum::Value<_enum_name> operator|(_enum_name l, _enum_name r)                                  \
    {                                                                                                         \
        return _enum_name(uint32_t(l) | uint32_t(r));                                                         \
    }                                                                                                         \
    constexpr xenum::Value<_enum_name> operator^(_enum_name l, _enum_name r)                                  \
    {                                                                                                         \
        return _enum_name(uint32_t(l) ^ uint32_t(r));                                                         \
    }                                                                                                         \
    constexpr bool HasFlag(_enum_name l, _enum_name r) { return (uint32_t(l) & uint32_t(r)) == uint32_t(r); } \
    constexpr xenum::Value<_enum_name> operator~(_enum_name t) { return _enum_name(~uint32_t(t)); }

// Declare an enumeration inside a class
#define XENUM(_enum_name, ...) XENUM_DETAIL_MAKE(class, enum, _enum_name, __VA_ARGS__)

// Declare an enumeration inside a namespace
#define XENUM_NS(_enum_name, ...) XENUM_DETAIL_MAKE(namespace, enum, _enum_name, __VA_ARGS__)

// Declare an enum class inside a class
#define XENUM_CLASS(_enum_name, ...)                              \
    XENUM_DETAIL_MAKE(class, enum class, _enum_name, __VA_ARGS__) \
    XENUM_OPS32(_enum_name)

// Provides access to information about an enum declared with XENUM or XENUM_NS
class XEnumReflector {
    struct Private {
        struct Enumerator {
            std::string name;
            int32_t     value = 0;
        };

        std::vector<Enumerator> values;
        std::string             enum_name;

        // TODO: (the common part of all enums values)
        std::string prefix;
    };

public:
    // On destructior set global flag
    class ICloseDetector {
    public:
        virtual ~ICloseDetector() = default;
    };

    template <class TEnum>
    class CloseDetectorImpl: public ICloseDetector {
        inline static bool closed_ = false;

    public:
        virtual ~CloseDetectorImpl() { closed_ = true; }

        static bool IsClosed() { return closed_; }
    };

    static bool IsIdentChar(char c)
    {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c == '_');
    }

private:
    Private*                        data_;
    std::unique_ptr<ICloseDetector> close_p_;

public:
    // Constructor. Used internally by XENUM and XENUM_NS
    XEnumReflector( // NOLINT(readability-function-cognitive-complexity)
        std::unique_ptr<ICloseDetector>&& _is_close_p,
        const int32_t*                    _vals_p,
        int32_t                           _count,
        const char*                       _name,
        const char*                       _body)
        : data_(new Private),
          close_p_(std::move(_is_close_p))

    {
        data_->enum_name = _name;
        data_->values.resize(_count);

        enum states {
            state_start, // Before identifier
            state_ident, // In identifier
            state_skip,  // Looking for separator comma
        } state = state_start;

        assert(*_body == '(');
        ++_body;
        const char* ident_start = nullptr;
        int32_t     value_index = 0;
        int32_t     level       = 0;
        for (;;) {
            assert(*_body);
            switch (state) {
                case state_start:
                    if (IsIdentChar(*_body)) {
                        state       = state_ident;
                        ident_start = _body;
                    }
                    ++_body;
                    break;
                case state_ident:
                    if (!IsIdentChar(*_body)) {
                        state = state_skip;
                        assert(value_index < _count);
                        data_->values[value_index].name  = std::string(ident_start, _body - ident_start);
                        data_->values[value_index].value = _vals_p[value_index];
                        ++value_index;
                    }
                    else {
                        ++_body;
                    }
                    break;
                case state_skip:
                    if (*_body == '(') {
                        ++level;
                    }
                    else if (*_body == ')') {
                        if (level == 0) {
                            assert(value_index == _count);
                            return;
                        }
                        --level;
                    }
                    else if (level == 0 && *_body == ',') {
                        state = state_start;
                    }
                    ++_body;
            }
        }
    }

    ~XEnumReflector()
    {
        if (data_)
            delete data_;

        close_p_.reset();
    }

    XEnumReflector(XEnumReflector&&) noexcept;

public:
    // Returns a reference to XEnumReflector object which can be used
    // to retrieve information about the enumeration declared with XENUM or XENUM_NS
    template <typename EnumType>
    static const XEnumReflector& For(EnumType _val = EnumType());

    template <typename EnumType>
    static bool IsClosed()
    {
        return CloseDetectorImpl<EnumType>::IsClosed();
    }

    // Represents an enumerator (value) of an enumeration
    class Enumerator {
    public:
        // Returns enumerator name
        const std::string& Name() const { return reflector_.data_->values[index_].name; }

        // Returns enumerator value
        int32_t Value() const { return reflector_.data_->values[index_].value; }

        // Returns enumerator index
        int32_t Index() const;

        // Returns parent reflector object
        const XEnumReflector& Reflector() const;

        // Check if this is an valid Enumerator
        bool IsValid() const;
             operator bool() const;

        // Check if two objects are the same
        bool operator!=(const Enumerator& rhs) const;

        // Moves on to the next Enumerator in enum
        Enumerator& operator++();

        // Provided for compatibility with range-based for construct
        const Enumerator& operator*() const;

    private:
        friend class XEnumReflector;
        Enumerator(const XEnumReflector&, int32_t);
        const XEnumReflector& reflector_;
        int32_t               index_;
    };

    // Returns Enumerator count
    int32_t Count() const { return (int32_t)data_->values.size(); }

    // Returns an Enumerator with specified name or invalid Enumerator if not found
    Enumerator Find(const std::string_view _name) const
    {
        for (int32_t i = 0; i < (int32_t)data_->values.size(); ++i) {
            if (data_->values[i].name == _name)
                return At(i);
        }
        return end();
    }

    // Returns an Enumerator with specified value or invalid Enumerator if not found
    Enumerator Find(int32_t value) const
    {
        for (int32_t i = 0; i < (int32_t)data_->values.size(); ++i) {
            if (data_->values[i].value == value)
                return At(i);
        }
        return end();
    }

    // Returns the enumeration name
    const std::string& EnumName() const { return data_->enum_name; }

    // Returns Enumerator at specified index
    Enumerator At(int32_t index) const;
    Enumerator operator[](int32_t index) const;

    // In some cases Enumerators can be used as iterators. The following functions
    // are provided e.g. for compatibility with range-based for construct:

    // Returns the first Enumerator
    Enumerator begin() const;

    // Returns an invalid Enumerator
    Enumerator end() const;
};

namespace xenum {

    template <class TEnum>
    constexpr bool HasFlag(const TEnum& _enum, const TEnum& _flag)
    {
        if (uint64_t(_flag) == 0)
            return uint64_t(_enum) == 0;

        return (uint64_t(_enum) & uint64_t(_flag)) == uint64_t(_flag);
    }

    // Use for ops
    template <typename T>
    struct Value {
        T t;

        constexpr Value(T t) : t(t) {}

        constexpr operator T() const { return t; }

        constexpr explicit operator bool() const { return uint32_t(t); }
    };

    template <class TEnum>
    std::string ToString(const TEnum&           _enum_val,
                         const std::string_view _default        = {},
                         const std::string_view _removed_prefix = {})
    {
        if (XEnumReflector::IsClosed<TEnum>())
            return "###Err### XENUM:" + std::string(xbase::TypeName<TEnum>()) + " Destroyed";

        auto int_val = static_cast<int32_t>(_enum_val);

        const auto& reflector = XEnumReflector::For<TEnum>();
        auto        enum_val  = reflector.Find(int_val);
        if (enum_val.IsValid()) {
            if (!_removed_prefix.empty()) {
                std::string wo_prefix;
                if (xbase::strings::StrIsPrefix(enum_val.Name(), _removed_prefix, &wo_prefix))
                    return wo_prefix;
            }

            return enum_val.Name();
        }

        if (!_default.empty())
            return std::string(_default);

        // Return 'EnumName(value)'
        return std::string(xbase::TypeName<TEnum>()) + "(" + std::to_string(int_val) + ")";
    }

    template <class TEnum>
    std::optional<TEnum> FromStringOne(const std::string_view _str_value, const std::optional<TEnum> _default = {})
    {
        if (XEnumReflector::IsClosed<TEnum>() || _str_value.empty())
            return _default;

        const auto& reflector = XEnumReflector::For<TEnum>();
        const auto  enum_val  = reflector.Find(_str_value);
        if (enum_val.IsValid())
            return static_cast<TEnum>(enum_val.Value());

        // Special fix for 'k' prefix
        if (_str_value[0] != 'k') {
            const auto enum_val_k = reflector.Find(std::string("k").append(_str_value));
            if (enum_val_k.IsValid())
                return static_cast<TEnum>(enum_val_k.Value());
        }

        return _default;
    }

    template <class TEnum>
    std::optional<TEnum> FromString(const std::string_view& _str_value, const std::optional<TEnum> _default = {})
    {
        std::optional<TEnum> result;
        for (const auto token : xbase::strings::StrSplit(_str_value, '|', true)) {
            auto enum_val = FromStringOne<TEnum>(token);
            if (enum_val.has_value())
                result = result.value_or(enum_val.value()) | enum_val.value();
        }

        return result;
    }

    template <class TEnum>
    inline TEnum FromString(const std::string_view _str_value, const TEnum& _default)
    {
        return FromString<TEnum>(_str_value).value_or(_default);
    }

} // namespace xenum

//----------------------------- Implementation Details -----------------------------

#define XENUM_DETAIL_SPEC_namespace                                   \
    extern "C" { /* Protection from being used inside a class body */ \
    }                                                                 \
    inline
#define XENUM_DETAIL_SPEC_class inline /*friend*/
#define XENUM_DETAIL_STR(x) #x
#define XENUM_DETAIL_MAKE(_spec, _enum_decl, _enum_name, ...)                                                    \
    _enum_decl                      _enum_name: int32_t {__VA_ARGS__};                                           \
    XENUM_DETAIL_SPEC_##_spec const xsdk::XEnumReflector& detail_reflector_(_enum_name)                          \
    {                                                                                                            \
        static const xsdk::XEnumReflector reflector([] {                                                         \
            static int32_t detail_sval;                                                                          \
            detail_sval = 0;                                                                                     \
            struct DetailVal {                                                                                   \
                DetailVal(const DetailVal& _rhs) : val_(_rhs) { detail_sval = val_ + 1; }                        \
                DetailVal(int32_t _val) : val_(_val) { detail_sval = val_ + 1; }                                 \
                DetailVal() : val_(detail_sval) { detail_sval = val_ + 1; }                                      \
                                                                                                                 \
                DetailVal& operator=(const DetailVal&) { return *this; }                                         \
                DetailVal& operator=(int32_t) { return *this; }                                                  \
                           operator int32_t() const { return val_; }                                             \
                int32_t    val_;                                                                                 \
            } __VA_ARGS__;                                                                                       \
            const int32_t detail_vals[] = {__VA_ARGS__};                                                         \
            return xsdk::XEnumReflector(std::make_unique<xsdk::XEnumReflector::CloseDetectorImpl<_enum_name>>(), \
                                        detail_vals,                                                             \
                                        sizeof(detail_vals) / sizeof(int32_t),                                   \
                                        #_enum_name,                                                             \
                                        XENUM_DETAIL_STR((__VA_ARGS__)));                                        \
        }());                                                                                                    \
        return reflector;                                                                                        \
    }

// XEnumReflector

inline XEnumReflector::XEnumReflector(XEnumReflector&& _rhs) noexcept
    : data_(_rhs.data_),
      close_p_(std::move(_rhs.close_p_))
{
    _rhs.data_ = nullptr;
}

template <typename EnumType>
inline const XEnumReflector& XEnumReflector::For(EnumType _val)
{
    return detail_reflector_(_val);
}

inline XEnumReflector::Enumerator XEnumReflector::At(int32_t index) const { return {*this, index}; }

inline XEnumReflector::Enumerator XEnumReflector::operator[](int32_t index) const { return At(index); }

inline XEnumReflector::Enumerator XEnumReflector::begin() const { return At(0); }

inline XEnumReflector::Enumerator XEnumReflector::end() const { return At(Count()); }

// XEnumReflector::Enumerator

inline XEnumReflector::Enumerator::Enumerator(const XEnumReflector& er, int32_t index) : reflector_(er), index_(index)
{
}

inline int32_t XEnumReflector::Enumerator::Index() const { return index_; }

inline const XEnumReflector& XEnumReflector::Enumerator::Reflector() const { return reflector_; }

inline XEnumReflector::Enumerator::operator bool() const { return IsValid(); }

inline bool XEnumReflector::Enumerator::IsValid() const { return index_ < reflector_.Count(); }

inline bool XEnumReflector::Enumerator::operator!=(const Enumerator& rhs) const { return index_ != rhs.index_; }

inline XEnumReflector::Enumerator& XEnumReflector::Enumerator::operator++()
{
    ++index_;
    return *this;
}

inline const XEnumReflector::Enumerator& XEnumReflector::Enumerator::operator*() const { return *this; }

} // namespace xsdk