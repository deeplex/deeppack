
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <concepts>
#include <limits>
#include <ranges>
#include <type_traits>
#include <utility>

#include <dplx/cncr/mp_lite.hpp>
#include <dplx/cncr/type_utils.hpp>
#include <dplx/cncr/utils.hpp>

////////////////////////////////////////////////////////////////////////////////
// dp::get CPO

namespace dplx::detail::cpo
{

template <std::size_t I, typename T>
void get(T &) = delete;
template <std::size_t I, typename T>
void get(T const &) = delete;

// clang-format off
template <typename T, std::size_t I>
concept has_tuple_get_adl = requires(T t)
{
    typename std::tuple_size<cncr::remove_cref_t<T>>::type;
    requires I < std::tuple_size_v<cncr::remove_cref_t<T>>;
    typename std::tuple_element<I, cncr::remove_cref_t<T>>::type;
    { get<I>(t) }
        -> std::convertible_to<
            std::conditional_t<std::is_const_v<std::remove_reference_t<T>>,
                std::tuple_element_t<I, cncr::remove_cref_t<T>> const &,
                std::tuple_element_t<I, cncr::remove_cref_t<T>> &>
            >;
};
// clang-format on

template <std::size_t I>
struct get_fn
{
    template <has_tuple_get_adl<I> T>
    DPLX_ATTR_FORCE_INLINE auto operator()(T &&t) const
            noexcept(noexcept(get<I>(t))) -> decltype(auto)
    {
        return get<I>(t);
    }
};

} // namespace dplx::detail::cpo

namespace dplx::dp
{

inline namespace cpo
{

template <std::size_t I>
inline constexpr ::dplx::detail::cpo::get_fn<I> get{};

}

} // namespace dplx::dp

////////////////////////////////////////////////////////////////////////////////
// tuple_like & pair_like

namespace dplx::dp::detail
{

template <typename T>
concept tuple_sized = requires
{
    typename std::tuple_size<T>::type;
};

template <typename F, typename T, std::size_t... Is>
constexpr auto
apply_simply_impl(F &&f, T &&t, std::index_sequence<Is...>) noexcept(
        noexcept(static_cast<F &&>(f)(dp::get<Is>(static_cast<T &&>(t))...)))
        -> decltype(auto)
{
    return static_cast<F &&>(f)(dp::get<Is>(static_cast<T &&>(t))...);
}
// a poor man's std::apply() which however uses unqualified get<I>()
// instead of std::get<I>(). This allows it to cope with custom tuple types.
template <typename F, typename T>
    requires tuple_sized<std::remove_cvref_t<T>>
constexpr auto
apply_simply(F &&f, T &&t) noexcept(noexcept(detail::apply_simply_impl<F, T>(
        static_cast<F &&>(f),
        static_cast<T &&>(t),
        std::make_index_sequence<
                std::tuple_size_v<std::remove_reference_t<T>>>())))
        -> decltype(auto)
{
    return detail::apply_simply_impl<F, T>(
            static_cast<F &&>(f), static_cast<T &&>(t),
            std::make_index_sequence<
                    std::tuple_size_v<std::remove_reference_t<T>>>());
}

template <typename T, typename IS>
struct tuple_element_list_deducer
{
};
template <typename T, std::size_t... Is>
struct tuple_element_list_deducer<T, std::index_sequence<Is...>>
{
    using type = cncr::mp_list<std::tuple_element_t<Is, T>...>;
};
template <typename T>
struct tuple_element_list
{
};
template <tuple_sized T>
struct tuple_element_list<T>
{
    using type = typename tuple_element_list_deducer<
            T,
            std::make_index_sequence<std::tuple_size_v<T>>>::type;
};

template <typename T>
using tuple_element_list_t = typename tuple_element_list<T>::type;

struct arg_sink
{
    template <typename... T>
    void operator()(T &&...) noexcept
    {
    }
};

} // namespace dplx::dp::detail

namespace dplx::dp
{

template <typename T>
concept pair_like = requires(T t)
{
    typename std::tuple_size<T>::type;
    requires 2U == std::tuple_size_v<T>;
    dp::get<0U>(t);
    dp::get<1U>(t);
};

// clang-format off
template <typename T>
concept tuple_like
    = !std::ranges::range<T>
    && detail::tuple_sized<T>
    && requires(T && t)
{
    typename detail::tuple_element_list<T>::type;
    detail::apply_simply(::dplx::dp::detail::arg_sink(), t);
};
// clang-format on

} // namespace dplx::dp

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
