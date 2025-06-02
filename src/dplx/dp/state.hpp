
// Copyright 2023 Henrik Steffen Gaßmann
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memory>
#include <memory_resource>
#include <utility>

#include <boost/unordered/unordered_flat_map.hpp>

#include <dplx/cncr/utils.hpp>
#include <dplx/cncr/uuid.hpp>

#include <dplx/dp/detail/workaround.hpp>
#include <dplx/dp/fwd.hpp>

namespace dplx::dp::detail
{

template <typename T, std::size_t PadSize>
struct erasure_pad
{
    T value;
    std::byte padding[PadSize];
};
template <typename T>
struct erasure_pad<T, 0U>
{
    T value;
};

template <typename To, typename From>
    requires(sizeof(From) < sizeof(To))
constexpr auto erasure_cast(From const &value) noexcept -> To
{
    return std::bit_cast<To>(
            erasure_pad<From, sizeof(To) - sizeof(From)>{value, {}});
}
template <typename To, typename From>
    requires(sizeof(From) == sizeof(To))
constexpr auto erasure_cast(From const &value) noexcept -> To
{
    return std::bit_cast<To>(value);
}

} // namespace dplx::dp::detail

namespace dplx::dp::detail
{

template <typename Allocator = std::pmr::polymorphic_allocator<std::byte>>
class unique_erased_state_ptr;

template <typename T, typename Allocator, typename... Args>
constexpr auto allocate_state(Allocator const &alloc, Args &&...args)
        -> unique_erased_state_ptr<Allocator>;

template <typename Allocator>
class unique_erased_state_ptr
{
    template <typename T, typename Alloc, typename... Args>
    friend constexpr auto allocate_state(Alloc const &alloc, Args &&...args)
            -> unique_erased_state_ptr<Alloc>;

    using delete_fn = void (*)(void *obj) noexcept;

    void *mObj{};
    delete_fn mDelete{};

    template <typename StateT>
    struct container
    {
        DPLX_ATTR_NO_UNIQUE_ADDRESS Allocator allocator;
        StateT state;

        using allocator_type = Allocator;

        constexpr container() = default;

        container(container const &) = delete;
        auto operator=(container const &) -> container & = delete;

        container(container &&) = delete;
        auto operator=(container &&) -> container & = delete;

        template <typename Arg, typename... Args>
            requires(!std::is_same_v<std::remove_cvref_t<Arg>,
                                     std::allocator_arg_t>)
        constexpr container(Arg &&arg, Args &&...args)
            : allocator()
            , state(static_cast<Arg &&>(arg), static_cast<Args &&>(args)...)
        {
        }
        template <typename... Args>
        constexpr container(std::allocator_arg_t,
                            Allocator const &alloc,
                            Args &&...args)
            : allocator(alloc)
            , state(std::make_obj_using_allocator<StateT, Allocator>(
                      alloc, static_cast<Args &&>(args)...))
        {
        }
    };

public:
    constexpr ~unique_erased_state_ptr() noexcept
    {
        if (mObj != nullptr && mDelete != nullptr)
        {
            mDelete(mObj);
        }
    }
    constexpr unique_erased_state_ptr() noexcept = default;

    constexpr unique_erased_state_ptr(unique_erased_state_ptr &&other) noexcept
        : mObj(std::exchange(other.mObj, nullptr))
        , mDelete(std::exchange(other.mDelete, nullptr))
    {
    }
#if DPLX_DP_WORKAROUND_ISSUE_LIBSTDCPP_108952
    constexpr unique_erased_state_ptr(unique_erased_state_ptr &other) noexcept
        : mObj(std::exchange(other.mObj, nullptr))
        , mDelete(std::exchange(other.mDelete, nullptr))
    {
    }
#endif
    constexpr auto operator=(unique_erased_state_ptr &&other) noexcept
            -> unique_erased_state_ptr &
    {
        mObj = std::exchange(other.mObj, nullptr);
        mDelete = std::exchange(other.mDelete, nullptr);

        return *this;
    }

private:
    template <typename StateT>
    constexpr explicit unique_erased_state_ptr(container<StateT> *obj)
        : mObj(static_cast<void *>(obj))
        , mDelete(&deleter<StateT>)
    {
    }

public:
    template <typename StateT>
    [[nodiscard]] auto get() const noexcept -> StateT *
    {
        return mObj == nullptr ? nullptr
                               : &static_cast<container<StateT> *>(mObj)->state;
    }

private:
    template <typename T>
    static void deleter(void *obj) noexcept
    {
        using allocator_type = typename std::allocator_traits<Allocator>::
                template rebind_alloc<typename unique_erased_state_ptr<
                        Allocator>::template container<T>>;
        using allocator_traits = typename std::allocator_traits<allocator_type>;

        auto *wrapped = static_cast<container<T> *>(obj);
        allocator_type allocator(wrapped->allocator);

        allocator_traits::destroy(allocator, wrapped);
        allocator_traits::deallocate(allocator, wrapped, 1U);
    }
};

template <typename T, typename Allocator, typename... Args>
constexpr auto allocate_state(Allocator const &alloc, Args &&...args)
        -> unique_erased_state_ptr<Allocator>
{
    using allocator_type =
            typename std::allocator_traits<Allocator>::template rebind_alloc<
                    typename unique_erased_state_ptr<
                            Allocator>::template container<T>>;
    using allocator_traits = typename std::allocator_traits<allocator_type>;

    allocator_type allocator(alloc);
    typename allocator_traits::pointer obj
            = allocator_traits::allocate(allocator, 1U);
    try
    {
        allocator_traits::construct(allocator, obj,
                                    static_cast<Args &&>(args)...);
        return unique_erased_state_ptr<Allocator>(obj);
    }
    catch (...)
    {
        allocator_traits::deallocate(allocator, obj, 1U);
        throw;
    }
}

} // namespace dplx::dp::detail

namespace dplx::dp::detail
{

template <typename Key, typename T>
using flat_hash_map = boost::unordered_flat_map<
        Key,
        T,
        std::hash<Key>,
        std::equal_to<>,
        std::pmr::polymorphic_allocator<std::pair<Key const, T>>>;

}

namespace dplx::dp
{

class state_base
{
public:
    virtual ~state_base() noexcept = default;

protected:
    constexpr state_base() noexcept = default;

    constexpr state_base(state_base const &) noexcept = default;
    constexpr auto operator=(state_base const &) noexcept
            -> state_base & = default;

    constexpr state_base(state_base &&) noexcept = default;
    constexpr auto operator=(state_base &&) noexcept -> state_base & = default;
};

template <typename StateT>
struct state_key
{
    cncr::uuid value{};

    using state_type = StateT;
};

class state_store
{
public:
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

private:
    using impl_type = detail::flat_hash_map<
            cncr::uuid,
            detail::unique_erased_state_ptr<allocator_type>>;
    impl_type mImpl{};

public:
    state_store() = default;

    state_store(allocator_type const &alloc)
        : mImpl(alloc)
    {
    }

    [[nodiscard]] auto get_allocator() const noexcept -> allocator_type
    {
        return mImpl.get_allocator();
    }
    void reserve(typename impl_type::size_type slots)
    {
        mImpl.reserve(slots);
    }

    [[nodiscard]] auto empty() const noexcept -> bool
    {
        return mImpl.empty();
    }
    template <typename StateT>
    [[nodiscard]] auto try_access(state_key<StateT> const &key) const noexcept
            -> StateT *
    {
        if (auto it = mImpl.find(key.value); it != mImpl.end())
        {
            return it->second.template get<StateT>();
        }
        return nullptr;
    }

    template <typename StateT, typename... Args>
    auto emplace(state_key<StateT> const &key, Args &&...args)
            -> std::pair<StateT *, bool>
    {
        auto hint = mImpl.find(key.value);
        if (hint != mImpl.end())
        {
            // this way, we don't need to allocate if key already has a value
            return {hint->second.template get<StateT>(), false};
        }
        auto it = mImpl.emplace_hint(
                hint, key.value,
                detail::allocate_state<StateT, allocator_type>(
                        mImpl.get_allocator(), static_cast<Args &&>(args)...));
        return {it->second.template get<StateT>(), true};
    }
    template <typename StateT>
    auto erase(state_key<StateT> const &key) noexcept -> std::size_t
    {
        return mImpl.erase(key.value);
    }
    auto erase(cncr::uuid key) noexcept -> std::size_t
    {
        return mImpl.erase(key);
    }
    void clear() noexcept
    {
        mImpl.clear();
    }
};

template <typename StateT>
    requires std::default_initializable<StateT>
class [[nodiscard]] scoped_state
{
    state_key<StateT> mKey;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    state_store &mStore;
    bool owned;

public:
    ~scoped_state() noexcept
    {
        if (owned)
        {
            mStore.erase(mKey.value);
        }
    }
    explicit scoped_state(state_store &store, state_key<StateT> const &key)
        : mKey(key)
        , mStore(store)
        , owned(false)
    {
        assert(!key.value.is_nil());
        owned = store.emplace(key).second;
    }

    [[nodiscard]] auto get() const noexcept -> StateT *
    {
        return mStore.try_access(mKey);
    }
};

template <typename T>
concept state_link = std::regular<T> && std::is_trivial_v<T>
                     && (sizeof(T) <= sizeof(cncr::uuid));

template <state_link T>
struct [[nodiscard]] state_link_key
{
    cncr::uuid value{};

    using link_type = T;
};

class link_store
{
public:
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

private:
    using erased_type
            = cncr::blob<std::byte, sizeof(cncr::uuid), alignof(cncr::uuid)>;
    using impl_type = detail::flat_hash_map<cncr::uuid, erased_type>;
    impl_type mImpl{};

public:
    link_store() = default;

    link_store(allocator_type const &alloc)
        : mImpl(alloc)
    {
    }

    [[nodiscard]] auto get_allocator() const noexcept -> allocator_type
    {
        return mImpl.get_allocator();
    }
    void reserve(typename impl_type::size_type slots)
    {
        mImpl.reserve(slots);
    }

    [[nodiscard]] auto empty() const noexcept -> bool
    {
        return mImpl.empty();
    }
    template <state_link T>
    [[nodiscard]] auto try_access(state_link_key<T> const &key) const noexcept
            -> T
    {
        if (auto it = mImpl.find(key.value); it != mImpl.end())
        {
            return std::bit_cast<detail::erasure_pad<T, sizeof(erased_type)
                                                                - sizeof(T)>>(
                           it->second)
                    .value;
        }
        return T{};
    }

    void clear() noexcept
    {
        mImpl.clear();
    }
    template <state_link T>
    auto replace(state_link_key<T> const &key, T value) -> T
    {
        auto it = mImpl.find(key.value);
        if (it == mImpl.end())
        {
            if (value != T{})
            {
                mImpl.emplace_hint(it, key.value,
                                   detail::erasure_cast<erased_type, T>(value));
            }
            return T{};
        }

        auto previousValue = std::bit_cast<detail::erasure_pad<
                T, sizeof(erased_type) - sizeof(T)>>(it->second)
                                     .value;
        if (value == T{})
        {
            mImpl.erase(it);
        }
        else
        {
            it->second = detail::erasure_cast<erased_type, T>(value);
        }
        return previousValue;
    }
};

template <state_link T>
class [[nodiscard]] scoped_link
{
    state_link_key<T> mKey;
    T mPreviousValue;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    link_store &mStore;

public:
    ~scoped_link() noexcept
    {
        mStore.replace(mKey, mPreviousValue);
    }
    explicit scoped_link(link_store &store,
                         state_link_key<T> const &key,
                         T value)
        : mKey(key)
        , mPreviousValue(store.replace(key, value))
        , mStore(store)
    {
    }

    [[nodiscard]] auto get() const noexcept -> T
    {
        return mStore.try_access(mKey);
    }
    [[nodiscard]] auto shadowed_value() const noexcept -> T
    {
        return mPreviousValue;
    }
};

} // namespace dplx::dp
