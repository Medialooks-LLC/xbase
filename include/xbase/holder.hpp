#pragma once

#include <any>
#include <utility>
#include <memory>
#include <cassert>

namespace xsdk::xbase {
/**
 * @brief Base template class for a holder pattern implementation.
 * @tparam TData Type of the data to be held by the holder.
 *
 * The Holder class is a template class that can hold any type of data TData
 * and an additional data holder. It can be used as a base class or as a
 * standalone class depending on the specific implementation.
 */
template <typename TData>
class Holder: public TData {
public:
    /**
     * @brief Default constructor.
     * This constructor is the default one. It does not initialize the holder
     * or the data.
     */
    Holder() = default;
    /**
     * @brief Move constructor from rvalue Holder.
     * This constructor is the move constructor from an rvalue Holder. It moves
     * the holder and the data from the rvalue Holder to the current object.
     */
    Holder(Holder&&) = default;
    /**
     * @brief Copy constructor from Holder.
     * This constructor is the copy constructor from a const Holder. It copies
     * the holder and the data from the const Holder to the current object.
     */
    Holder(const Holder&) = default;
    /**
     * @brief Constructor from std::pair.
     * This constructor initializes the holder and the data with the data and
     * the holder from a std::pair.
     * @param _data_n_holder A std::pair that contains the data and the holder.
     */
    Holder(std::pair<TData, std::any>&& _data_n_holder)
        : TData(std::move(_data_n_holder.first)),
          holder_(std::move(_data_n_holder.second))
    {
    }
    /**
     * @brief Constructor from data and holder.
     * This constructor initializes the holder and the data with the provided
     * data and the holder.
     * @param _data The data to be held by the Holder.
     * @param _data_holder The holder of the data.
     */
    Holder(TData&& _data, std::any&& _data_holder) : TData(std::move(_data)), holder_(std::move(_data_holder)) {}
    /**
     * @brief Constructor from data and holder.
     * This constructor initializes the holder and the data with the provided
     * data and the holder.
     * @param _data The data to be held by the Holder.
     * @param _data_holder The holder of the data.
     */
    Holder(const TData& _data, std::any&& _data_holder) : TData(_data), holder_(std::move(_data_holder)) {}

private:
    std::any holder_;
};

/**
 * @brief A wrapper class for a Holder with a public interface for the data.
 * @tparam TData Type of the data to be held by the wrapper.
 *
 * The HolderP class is a wrapper around the Holder template class, which
 * provides a public interface for the data and adds some useful methods.
 */
template <typename TData>
class HolderP {
public:
    /**
     * @brief Default constructor.
     * This constructor does not initialize the data and the holder.
     */
    HolderP() = default;
    /**
     * @brief Move constructor from rvalue HolderP.
     * This constructor moves the data and the holder from an rvalue HolderP.
     */
    HolderP(HolderP&&) = default;
    /**
     * @brief Copy constructor from const HolderP.
     * This constructor copies the data and the holder from a const HolderP.
     */
    HolderP(const HolderP& _copy) : holder_(_copy.holder_)
    {
        if (_copy.DataPtr())
            data_p_ = std::make_unique<TData>(_copy.Data());
    }
    /**
     * @brief Copy constructor from const HolderP with convertable to TData type.
     * This constructor copies the data and the holder from a const HolderP.
     */
    template <typename TConvertFrom>
    HolderP(const HolderP<TConvertFrom>& _copy) : holder_(_copy.Holder())
    {
        static_assert(std::is_convertible_v<TConvertFrom, TData>);
        if (_copy.DataPtr())
            data_p_ = std::make_unique<TData>(_copy.Data());
    }
    /**
     * @brief Constructor from std::pair.
     * This constructor initializes the data and the holder with the data and
     * the holder from a std::pair.
     * @param _data_n_holder A std::pair that contains the data and the holder.
     */
    HolderP(std::pair<std::unique_ptr<TData>, std::any>&& _data_n_holder)
        : data_p_(std::move(_data_n_holder.first)),
          holder_(std::move(_data_n_holder.second))
    {
    }
    /**
     * @brief Constructor from data and holder.
     * This constructor initializes the data and the holder with the provided
     * data and the holder.
     * @param _data The data to be held by the wrapper.
     * @param _data_holder The holder of the data.
     */
    HolderP(TData&& _data, std::any&& _data_holder)
        : data_p_(std::make_unique<TData>(std::move(_data))),
          holder_(std::move(_data_holder))
    {
    }
    /**
     * @brief Constructor from data and holder.
     * This constructor initializes the data and the holder with the provided
     * data and the holder.
     * @param _data The data to be held by the wrapper.
     * @param _data_holder The holder of the data.
     */
    HolderP(const TData& _data, std::any&& _data_holder)
        : data_p_(std::make_unique<TData>(_data)),
          holder_(std::move(_data_holder))
    {
    }

    HolderP& operator=(HolderP&&) = default;
    HolderP& operator=(const HolderP& _copy)
    {
        if (_copy.DataPtr())
            data_p_ = std::make_unique<TData>(_copy.Data());
        holder_ = _copy.holder_;
        return *this;
    }
    template <typename TConvertFrom>
    HolderP& operator=(const HolderP<TConvertFrom>& _copy)
    {
        static_assert(std::is_convertible_v<TConvertFrom, TData>);
        if (_copy.DataPtr())
            data_p_ = std::make_unique<TData>(_copy.Data());
        holder_ = _copy.Holder();
        return *this;
    }
    /**
     * @brief Check is holder an empty
     */
    bool IsEmpty() const { return !data_p_; }
    /**
     * @brief Const accessor for the data.
     * @return A const reference to the data.
     */
    const TData& Data() const
    {
        assert(data_p_);
        return *data_p_;
    }
    /**
     * @brief Const accessor for the data pointer
     * @return A const pointer to the data.
     */
    const TData* DataPtr() const { return data_p_.get(); }

    /**
     * @brief Implicit conversion to const TData.
     * This implicit conversion operator converts the wrapper to a const TData.
     * @return A const TData.
     */
    operator const TData&() const { return Data(); }
    /**
     * @brief Pointer to the data.
     * @return A pointer to the data.
     */
    const TData* operator->() const { return data_p_.get(); }
    /**
     * @brief Equality comparison with a TData.
     * This operator compares the data of the wrapper with a TData.
     * @param val The TData to be compared with.
     * @return true if the data of the wrapper is equal to the TData, false otherwise.
     */
    bool operator==(const TData& val) const { return data_p_ && *data_p_ == val; }

    /**
     * @brief This method returns holder 
     * @return The copy of holder.
     */
    std::any Holder() const { return holder_; }

    /**
     * @brief Detach method.
     * This method detaches the data and the holder from the wrapper and returns
     * them as a std::pair.
     * @return A std::pair that contains the data and the holder.
     */
    std::pair<std::unique_ptr<TData>, std::any> Detach()
    {
        return {std::exchange(data_p_, std::unique_ptr<TData>()), std::move(holder_)};
    }

private:
    std::unique_ptr<TData> data_p_;
    std::any               holder_;
};

} // namespace xsdk::xbase
