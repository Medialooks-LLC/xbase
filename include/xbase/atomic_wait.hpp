#pragma once

#include <atomic>
#include <cassert>
#include <cmath>
#include <condition_variable>
#include <functional>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <utility>
#include <system_error>

#include "xpointers.hpp"

namespace xsdk::xbase {

template <class TData>
class AtomicWait {
public:
    USING_PTRS(AtomicWait<TData>)

    using IsSuitablePf = const std::function<bool(const TData _current, const bool _initial_value)>;
    using MutexSPtr    = std::shared_ptr<std::shared_mutex>;

public:
    std::atomic<bool>                   is_closed_ {false};
    TData                               value_;
    mutable std::condition_variable_any cv_wait_;
    const MutexSPtr                     mtx_p_;
    const AtomicWait<TData>::WPtr       notify_parent_wp_;

public:
    AtomicWait(const TData& _value = {}, const MutexSPtr& _shared_mutex_p = {})
        : value_(_value),
          mtx_p_(_shared_mutex_p ? _shared_mutex_p : std::make_shared<std::shared_mutex>())
    {
    }

    template <class TOtherData>
    AtomicWait(const AtomicWait<TOtherData>* _for_shared_mutex, const TData& _value = {})
        : value_(_value),
          mtx_p_(_for_shared_mutex ? _for_shared_mutex->MutexPtr() : std::make_shared<std::shared_mutex>())
    {
    }

    AtomicWait(const AtomicWait<TData>::SPtr& _for_notify_parent, const TData& _value = {})
        : value_(_value),
          mtx_p_(_for_notify_parent ? _for_notify_parent->MutexPtr() : std::make_shared<std::shared_mutex>()),
          notify_parent_wp_(_for_notify_parent)
    {
    }

    static AtomicWait<TData>::SPtr Create(const AtomicWait<TData>::SPtr& _for_notify_parent = {},
                                          const TData                    _value             = {})
    {
        return std::make_shared<AtomicWait<TData>>(_for_notify_parent, _value);
    }

    ~AtomicWait() { Close(TData {}); }

    TData SetValue(const TData _value, const bool _notify_always)
    {
        std::unique_lock lck(*mtx_p_);

        auto prev = std::exchange(value_, _value);
        if (!_notify_always && prev == _value)
            return prev;

        cv_wait_.notify_all();

        lck.unlock();

        ParentSetValue_(_value, _notify_always, this);

        return prev;
    }

    const MutexSPtr& MutexPtr() const { return mtx_p_; }

    std::shared_mutex& Mutex() const { return *mtx_p_; }

    TData Value() const
    {
        const std::shared_lock lck(*mtx_p_);
        return value_;
    }

    bool Close(const std::optional<TData> _parent_set)
    {
        auto closed = is_closed_.exchange(true);

        if (_parent_set.has_value())
            ParentSetValue_(_parent_set.value(), true, this);

        cv_wait_.notify_all();
        return closed;
    }

    std::pair<std::error_code, TData> WaitValuePf(const IsSuitablePf&         _is_suitable_pf,
                                                  const std::optional<double> _wait_seconds = {}) const
    {
        assert(_is_suitable_pf);
        if (!_is_suitable_pf)
            return {std::make_error_code(std::errc::function_not_supported), Value()};

        auto val = Value();

        bool is_initial_val = true;
        if (_wait_seconds.value_or(1.0) > 0.0 && !is_closed_.load() &&
            !_is_suitable_pf(val, std::exchange(is_initial_val, false))) {

            std::unique_lock lck(Mutex());

            if (_wait_seconds.has_value()) {
                cv_wait_.wait_for(lck, std::chrono::duration<double>(_wait_seconds.value()), [&]() {
                    return is_closed_.load() ? true : _is_suitable_pf(value_, false);
                });
            }
            else {
                cv_wait_.wait(lck, [&]() { return is_closed_.load() ? true : _is_suitable_pf(value_, false); });
            }

            val = value_;
        }

        if (_is_suitable_pf(val, std::exchange(is_initial_val, false)))
            return {std::error_code {}, val};

        return {_wait_seconds.has_value() ? std::make_error_code(std::errc::timed_out) :
                                            std::make_error_code(std::errc::not_supported),
                val};
    }

    std::pair<std::error_code, TData> WaitForChanges(const std::optional<double> _wait_seconds = {})
    {
        return WaitValuePf(
            [start_val = Value()](const TData _check, const bool _is_initial) -> bool {
                return !_is_initial && _check != start_val;
            },
            _wait_seconds);
    }

    std::pair<std::error_code, TData> WaitForChanges(const std::optional<double> _wait_seconds   = {},
                                                     const TData&                _not_wait_value = {})
    {
        return WaitValuePf(
            [_not_wait_value](const TData _check, const bool _is_initial) -> bool {
                return !_is_initial || _check == _not_wait_value;
            },
            _wait_seconds);
    }

    std::pair<std::error_code, TData> WaitValue(const TData& _expected, const std::optional<double> _wait_seconds = {})
    {
        return WaitValuePf(
            [_expected](const TData _check, const bool _is_initial) -> bool { return _check == _expected; },
            _wait_seconds);
    }

private:
    bool ParentSetValue_(const TData              _value, // NOLINT(misc-no-recursion)
                         const bool               _notify_always,
                         const AtomicWait<TData>* _stop_p)
    {
        auto notify_parent_sp = notify_parent_wp_.lock();
        if (!notify_parent_sp)
            return false;

        assert(notify_parent_sp.get() != _stop_p);
        if (notify_parent_sp.get() == _stop_p)
            return false;

        notify_parent_sp->ParentSetValue_(_value, _notify_always, _stop_p);
        return true;
    }
};

} // namespace xsdk::xbase
