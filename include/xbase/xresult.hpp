#pragma once

#include <memory>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <variant>

namespace xsdk::xbase {
/**
 * @brief Helper template to check if given type is a smart pointer
 * @tparam T The type to check
 */
template <typename T>
struct is_smart_ptr: std::false_type {};
/**
 * @brief Specialization for std::shared_ptr
 * @tparam T The pointed type for shared_ptr
 */
template <typename T>
struct is_smart_ptr<std::shared_ptr<T>>: std::true_type {};
/**
 * @brief Specialization for std::unique_ptr
 * @tparam T The pointed type for unique_ptr
 */
template <typename T>
struct is_smart_ptr<std::unique_ptr<T>>: std::true_type {};

/**
 * @brief A variadic type that can hold either a monostate, a result, or an error code.
 * @tparam TResult The type of the result.
 *
 */
template <typename TResult>
using XResultVariant = std::variant<std::monostate, TResult, std::error_code>;

/**
 * @brief Template class representing a result of a function call, which can contain either a result, an error or null
 * state.
 * @tparam TResult The type of the result, can be a plain object, smart pointer or error_code.
 * @see XResultVariant
 * @details
 * @par Usage:
 * @par For error:
 *      ```return std::error_code {my_code};```
 * @par For success:
 *      ```return result;```
 */
template <typename TResult>
class XResult final: public XResultVariant<TResult> {
    std::unique_ptr<const std::string> description_p_;

public:
    /// @brief Constructor for a result in empty state.
    XResult() = default;
    /// @brief Move constructor.
    XResult(XResult&&) = default;
    /// @brief Copy constructor.
    XResult(const XResult& _copy) : XResultVariant<TResult>(_copy)
    {
        if (_copy.description_p_)
            description_p_ = std::make_unique<std::string>(*_copy.description_p_);
    }
    /**
     *  @brief Constructor for a result containing an error value.
     *  @param _err The error to be stored.
     */
    XResult(std::error_code _err) : std::variant<std::monostate, TResult, std::error_code>(_err) {}
    /**
     *  @brief Move constructor for a result containing some value.
     *  @param _res The result to be stored.
     */
    XResult(TResult&& _res) : std::variant<std::monostate, TResult, std::error_code>(std::move(_res)) {}
    /**
     *  @brief Copy constructor for a result containing some value.
     *  @param _res The result to be stored.
     */
    XResult(const TResult& _res) : std::variant<std::monostate, TResult, std::error_code>(_res) {}
    /**
     * @brief Constructor for a result containing an extended error.
     * @param _extended_err A pair of error_code and a string describing the error.
     */
    XResult(std::pair<std::error_code, std::string_view>&& _extended_err)
        : std::variant<std::monostate, TResult, std::error_code>(_extended_err.first),
          description_p_(std::make_unique<std::string>(_extended_err.second))
    {
    }
    /// @brief Constructor for a null state.
    XResult(std::nullptr_t) : std::variant<std::monostate, TResult, std::error_code>(TResult(nullptr)) {}
    /**
     * @brief Constructor for a result containing a shared_ptr.
     * @tparam TInterface The interface type of the smart pointer.
     * @param _sp The smart pointer to be moved into the result object.
     */
    template <typename TInterface>
    XResult(std::shared_ptr<TInterface>&& _sp)
        : std::variant<std::monostate, TResult, std::error_code>(TResult(std::move(_sp)))
    {
    }
    /**
     * @brief Constructor for a result containing a unique_ptr.
     * @tparam TInterface The interface type of the smart pointer.
     * @param _up The smart pointer to be moved into the result object.
     */
    template <typename TInterface>
    XResult(std::unique_ptr<TInterface>&& _up)
        : std::variant<std::monostate, TResult, std::error_code>(TResult(std::move(_up)))
    {
    }
    /**
     * @brief Constructor for a result containing a custom error.
     * @tparam TError The error type.
     * @param _error The error object to be stored.
     */
    template <typename TError>
    XResult(TError _error) : std::variant<std::monostate, TResult, std::error_code>(std::error_code(_error))
    {
    }

    XResult& operator=(XResult&&) = default;
    XResult& operator=(const XResult& _copy)
    {
        XResultVariant<TResult>(*this) = _copy;
        if (_copy.description_p_)
            description_p_ = std::make_unique<std::string>(*_copy.description_p_);
        else
            description_p_.reset();
        return *this;
    }

    /**
     * @brief Checks if the result is in the empty state.
     * @return true if the result is empty, false otherwise.
     */
    [[nodiscard]] bool Empty() const { return std::get_if<std::monostate>(this) ? true : false; }
    /**
     * @brief Checks if the result contains a result value.
     * @return true if the result contains a result value, false otherwise.
     */
    [[nodiscard]] bool HasResult() const { return std::get_if<TResult>(this) ? true : false; }
    /**
     * @brief Checks if the result contains an error.
     * @return true if the result contains an error, false otherwise.
     */
    [[nodiscard]] bool HasError() const { return std::get_if<std::error_code>(this) ? true : false; }

    /**
     * @brief Returns the error if the result contains an error, or the provided default error if it does not.
     * @param _code_if_noerror The default error to return if the result is empty or contains a result.
     * @return The error code or the provided default error.
     */
    [[nodiscard]] std::error_code Error(const std::error_code _code_if_noerror = {}) const
    {
        const auto* err_p = std::get_if<std::error_code>(this);
        return (err_p && *err_p) ? *err_p : _code_if_noerror;
    }

    /**
     * @brief Returns the description of the error if the result contains an error, or an empty string if it does not.
     * @return The description of the error or an empty string.
     */
    [[nodiscard]] std::string_view Description() const { return description_p_ ? *description_p_ : std::string_view(); }

    /**
     * @brief Returns the result if the result contains a result value, or the provided default result if it does not.
     * @param _for_error The default result to return if the result is empty or contains an error.
     * @return The result or the provided default result.
     */
    [[nodiscard]] const TResult& Result(const TResult& _for_error = {}) const
    {
        const auto* res_p = std::get_if<TResult>(this);
        return res_p ? *res_p : _for_error;
    }
    /**
     * @brief Moves the result value from the result object and returns it.
     * @param _for_error The default result to return if the result is empty or contains an error.
     * @return The moved result value or the provided default result.
     */
    TResult MoveResult(TResult&& _for_error = {})
    {
        auto* res_p = std::get_if<TResult>(this);
        if (res_p)
            return std::exchange(*res_p, TResult());

        return std::move(_for_error);
    }

    // For structures inside
    template <typename T = TResult, std::enable_if_t<!is_smart_ptr<T>::value, bool> = true>
    T* operator->()
    {
        auto* res_p = std::get_if<T>(this);
        assert(res_p);
        if (res_p)
            return res_p;

        return nullptr;
    }

    // For shared_ptr/unique_ptr
    template <typename T = TResult, std::enable_if_t<is_smart_ptr<T>::value, bool> = true>
    typename T::element_type* operator->()
    {
        auto* res_p = std::get_if<T>(this);
        assert(res_p);
        if (res_p)
            return res_p->get();

        return nullptr;
    }

    template <typename T = TResult, std::enable_if_t<is_smart_ptr<T>::value, bool> = true>
    [[nodiscard]] typename T::element_type* GetPtr()
    {
        auto* res_p = std::get_if<T>(this);
        if (res_p)
            return res_p->get();

        return nullptr;
    }
};

} // namespace xsdk::xbase
