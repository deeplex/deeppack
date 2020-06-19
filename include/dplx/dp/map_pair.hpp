
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <utility>

namespace dplx::dp
{

template <typename Key, typename Value>
struct map_pair final
{
    using key_type = Key;
    using value_type = Value;

    key_type key;
    value_type value;

    bool operator==(map_pair const &) const noexcept = default;
    auto operator<=>(map_pair const &) const noexcept = default;
};

template <typename Key, typename Value>
map_pair(Key, Value) -> map_pair<Key, Value>;

template <std::size_t I, typename Key, typename Value>
inline decltype(auto) get(map_pair<Key, Value> &mapPair) noexcept
{
    static_assert(I < 2);
    if constexpr (I == 0)
    {
        return static_cast<Key &>(mapPair.key);
    }
    else
    {
        return static_cast<Value &>(mapPair.value);
    }
}
template <std::size_t I, typename Key, typename Value>
inline decltype(auto) get(map_pair<Key, Value> const &mapPair) noexcept
{
    static_assert(I < 2);
    if constexpr (I == 0)
    {
        return static_cast<Key const &>(mapPair.key);
    }
    else
    {
        return static_cast<Value const &>(mapPair.value);
    }
}
template <std::size_t I, typename Key, typename Value>
inline decltype(auto) get(map_pair<Key, Value> &&mapPair) noexcept
{
    static_assert(I < 2);
    if constexpr (I == 0)
    {
        return static_cast<Key &&>(mapPair.key);
    }
    else
    {
        return static_cast<Value &&>(mapPair.value);
    }
}
template <std::size_t I, typename Key, typename Value>
inline decltype(auto) get(map_pair<Key, Value> const &&mapPair) noexcept
{
    static_assert(I < 2);
    if constexpr (I == 0)
    {
        return static_cast<Key const &&>(mapPair.key);
    }
    else
    {
        return static_cast<Value const &&>(mapPair.value);
    }
}

} // namespace dplx::dp

namespace std
{

template <typename Key, typename Value>
struct tuple_size<dplx::dp::map_pair<Key, Value>>
    : ::std::integral_constant<std::size_t, 2>
{
};

template <typename Key, typename Value>
class tuple_element<0, dplx::dp::map_pair<Key, Value>>
{
public:
    using type = Key;
};
template <typename Key, typename Value>
class tuple_element<1, dplx::dp::map_pair<Key, Value>>
{
public:
    using type = Value;
};

} // namespace std
