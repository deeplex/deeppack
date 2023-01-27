
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

////////////////////////////////////////////////////////////////////////////////
// member accessor utilities

namespace dplx::dp
{

// clang-format off
template <typename T>
concept member_accessor
    = std::is_default_constructible_v<T>
    && requires(
        T &&x, typename T::class_type &ref, typename T::class_type const &cref)
{
    { x(ref) }
        -> std::same_as<typename T::value_type *>;
    { x(cref) }
        -> std::same_as<typename T::value_type const *>;
};
// clang-format on

template <typename ClassType, typename ValueType>
struct member_accessor_base
{
    using class_type = ClassType;
    using value_type = ValueType;
};

} // namespace dplx::dp

namespace dplx::dp::detail
{

template <typename T>
struct member_object_pointer_type_traits
{
};

template <typename T, typename C>
struct member_object_pointer_type_traits<T C::*>
{
    using type = T C::*;
    using value_type = T;
    using class_type = C;
};

template <typename T>
using member_object_pointer_value_type =
        typename member_object_pointer_type_traits<T>::value_type;

template <typename T>
using member_object_pointer_class_type =
        typename member_object_pointer_type_traits<T>::class_type;

template <typename T>
struct covariance_combiner
{
    using type = T;
};

template <typename T>
auto operator,(covariance_combiner<T>, covariance_combiner<T>)
                      -> covariance_combiner<T>;
template <typename T, typename U>
    requires(std::is_base_of_v<T, U>)
auto operator,(covariance_combiner<T>, covariance_combiner<U>)
                      -> covariance_combiner<U>;
template <typename T, typename U>
    requires(std::is_base_of_v<U, T>)
auto operator,(covariance_combiner<T>, covariance_combiner<U>)
                      -> covariance_combiner<T>;

template <typename... Ts>
struct covariance
{
    using type = typename decltype((..., covariance_combiner<Ts>{}))::type;
};

template <typename... Ts>
using covariance_t = typename covariance<Ts...>::type;

template <typename T>
struct is_type_identity : std::false_type
{
};
template <typename T>
struct is_type_identity<std::type_identity<T>> : std::true_type
{
};

} // namespace dplx::dp::detail
