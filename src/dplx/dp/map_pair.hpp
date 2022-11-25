
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <utility>

#include <dplx/predef/compiler/clang.h>

#include <dplx/dp/detail/workaround.hpp>

namespace dplx::dp
{

template <typename Key, typename Value>
struct map_pair final
{
    using key_type = Key;
    using value_type = Value;

    key_type key;
    value_type value;

    friend constexpr auto operator==(map_pair const &,
                                     map_pair const &) noexcept -> bool
            = default;
    friend constexpr auto operator<=>(map_pair const &,
                                      map_pair const &) noexcept = default;

    template <std::size_t I>
        requires(I <= 1U)
    friend constexpr auto get(map_pair &mapPair) noexcept -> decltype(auto)
    {
        if constexpr (I == 0U)
        {
            return static_cast<Key &>(mapPair.key);
        }
        else
        {
            return static_cast<Value &>(mapPair.value);
        }
    }
    template <std::size_t I>
        requires(I <= 1U)
    friend constexpr auto get(map_pair const &mapPair) noexcept
            -> decltype(auto)
    {
        if constexpr (I == 0U)
        {
            return static_cast<Key const &>(mapPair.key);
        }
        else
        {
            return static_cast<Value const &>(mapPair.value);
        }
    }
    template <std::size_t I>
        requires(I <= 1U)
    friend constexpr auto get(map_pair &&mapPair) noexcept -> decltype(auto)
    {
        if constexpr (I == 0U)
        {
            return static_cast<Key &&>(mapPair.key);
        }
        else
        {
            return static_cast<Value &&>(mapPair.value);
        }
    }
    template <std::size_t I>
        requires(I <= 1U)
    friend constexpr auto get(map_pair const &&mapPair) noexcept
            -> decltype(auto)
    {
        if constexpr (I == 0U)
        {
            return static_cast<Key const &&>(mapPair.key);
        }
        else
        {
            return static_cast<Value const &&>(mapPair.value);
        }
    }
};

#if DPLX_DP_WORKAROUND_TESTED_AT(DPLX_COMP_CLANG, 16, 0, 0)
// clang doesn't support class template argument deduction for aggreggates
template <typename Key, typename Value>
map_pair(Key, Value) -> map_pair<Key, Value>;
#endif

} // namespace dplx::dp

template <typename Key, typename Value>
struct std::tuple_size<dplx::dp::map_pair<Key, Value>>
    : integral_constant<std::size_t, 2U>
{
};

template <typename Key, typename Value>
class std::tuple_element<0U, dplx::dp::map_pair<Key, Value>>
{
public:
    using type = Key;
};
template <typename Key, typename Value>
class std::tuple_element<1U, dplx::dp::map_pair<Key, Value>>
{
public:
    using type = Value;
};
