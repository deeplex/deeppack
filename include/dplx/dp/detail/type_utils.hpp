
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

#include <boost/predef/compiler.h>

#include <dplx/dp/detail/mp_lite.hpp>
#include <dplx/dp/detail/workaround.hpp>

namespace dplx::dp::detail
{

template <typename T, typename... Ts>
concept any_of = (std::same_as<T, Ts> || ...);

template <typename T, typename... Ts>
concept none_of = (!std::same_as<T, Ts> && ...);

} // namespace dplx::dp::detail

namespace dplx::dp
{

template <typename T>
concept integer = std::integral<T> && detail::none_of < std::remove_cv_t<T>,
bool, char, wchar_t, char8_t, char16_t, char32_t > ;

template <typename T>
concept signed_integer = integer<T> && std::is_signed_v<T>;

template <typename T>
concept unsigned_integer = integer<T> && !signed_integer<T>;

// clang-format off
template <typename T>
concept iec559_floating_point
    = std::is_floating_point_v<T>
    && (sizeof(T) == 4 || sizeof(T) == 8)
    && requires
    {
        typename std::numeric_limits<T>;

#if DPLX_DP_WORKAROUND(BOOST_COMP_MSVC, <, 19, 28, 0) 
        // MSVC concepts implementation bug | recheck every version.
        // e.g. iec559_floating_point<int[2]> causes a
        // C2090: function returns array
        // compilation error in the fallback std::numeric_limits<T>
        // implementation.
#else
        requires std::numeric_limits<T>::is_iec559;
#endif
    };
// clang-format on

// gcc 10.1 is a bit too eager looking up get and needs a little assurance
// that everything is alrighty
template <std::size_t>
void get(...) noexcept = delete;

template <typename T>
concept pair_like = requires(T &&t)
{
    typename std::tuple_size<T>::type;
    requires std::derived_from<std::tuple_size<T>,
                               std::integral_constant<std::size_t, 2>>;
    typename std::tuple_element<0, T>::type;
    typename std::tuple_element<1, T>::type;
    {
        get<0>(t)
        } -> std::convertible_to<std::tuple_element_t<0, T> &>;
    {
        get<1>(t)
        } -> std::convertible_to<std::tuple_element_t<1, T> &>;
};

template <typename T>
concept member_accessor = std::is_default_constructible_v<T> && requires(
        T &&x, typename T::class_type &ref, typename T::class_type const &cref)
{
    {
        x(ref)
        } -> std::same_as<typename T::value_type *>;
    {
        x(cref)
        } -> std::same_as<typename T::value_type const *>;
};

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

template <std::size_t N, typename = std::make_index_sequence<N>>
struct nth_param_type_impl;

template <std::size_t N, std::size_t... Is>
struct nth_param_type_impl<N, std::index_sequence<Is...>>
{
    template <typename T>
    static T deduce(std::void_t<decltype(Is)> *..., T *, ...);
};

template <std::size_t N, typename... Params>
using nth_param_t = typename decltype(nth_param_type_impl<N>::deduce(
        static_cast<std::type_identity<Params> *>(nullptr)...))::type;

#if DPLX_DP_WORKAROUND_TESTED_AT(BOOST_COMP_MSVC, 19, 29, 30137)

template <std::size_t N, auto... Vs>
struct nth_param_value_impl;

template <auto V0, auto... Vs>
struct nth_param_value_impl<0, V0, Vs...>
{
    static constexpr auto value = V0;
};

template <std::size_t N, auto V0, auto... Vs>
struct nth_param_value_impl<N, V0, Vs...> : nth_param_value_impl<N - 1, Vs...>
{
};

template <std::size_t N, auto... Vs>
inline constexpr auto &nth_param_v = nth_param_value_impl<N, Vs...>::value;

#else

template <typename>
struct discarder
{
    template <typename T>
    constexpr discarder(T &&) noexcept
    {
    }
};

template <std::size_t N, typename = std::make_index_sequence<N>>
struct nth_param_value_impl;

template <std::size_t N, std::size_t... Is>
struct nth_param_value_impl<N, std::index_sequence<Is...>>
{
    template <typename T>
    static constexpr decltype(auto)
    access(discarder<decltype(Is)>..., T &&t, ...) noexcept
    {
        return static_cast<T &&>(t);
    }
};

template <std::size_t N, decltype(auto)... Vs>
inline constexpr auto &nth_param_v = nth_param_value_impl<N>::access(Vs...);

#endif

template <typename T>
concept tuple_sized = requires
{
    typename std::tuple_size<T>::type;
};

template <typename F, typename T, std::size_t... Is>
constexpr decltype(auto)
apply_simply_impl(F &&f, T &&t, std::index_sequence<Is...>) noexcept(
        noexcept(static_cast<F &&>(f)(get<Is>(static_cast<T &&>(t))...)))
{
    return static_cast<F &&>(f)(get<Is>(static_cast<T &&>(t))...);
}
// a poor man's std::apply() which however uses unqualified get<I>()
// instead of std::get<I>(). This allows it to cope with custom tuple types.
template <typename F, typename T>
    requires tuple_sized<std::remove_cvref_t<T>>
constexpr decltype(auto)
apply_simply(F &&f, T &&t) noexcept(noexcept(detail::apply_simply_impl(
        static_cast<F &&>(f),
        static_cast<T &&>(t),
        std::make_index_sequence<
                std::tuple_size_v<std::remove_reference_t<T>>>())))
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
    using type = mp_list<std::tuple_element_t<Is, T>...>;
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

// clang-format off
template <typename T>
concept tuple_like
    = !std::ranges::range<T> && tuple_sized<T> &&
        requires(T && t)
        {
            typename tuple_element_list<T>::type;
            ::dplx::dp::detail::apply_simply(::dplx::dp::detail::arg_sink(), t);
        };
// clang-format on

template <dp::unsigned_integer T>
constexpr T div_ceil(T dividend, T divisor)
{
    return dividend / divisor + (dividend % divisor != 0);
}
template <dp::unsigned_integer T, dp::unsigned_integer U>
constexpr auto div_ceil(T dividend, U divisor) -> std::common_type_t<T, U>
{
    using common_t = std::common_type_t<T, U>;
    return detail::div_ceil<common_t>(static_cast<common_t>(dividend),
                                      static_cast<common_t>(divisor));
}

template <typename Enum>
constexpr auto to_underlying(Enum value) noexcept ->
        typename std::underlying_type<Enum>::type
{
    return static_cast<std::underlying_type_t<Enum>>(value);
}

template <typename... Ts>
struct contravariance;

template <typename... Ts>
using contravariance_t = typename contravariance<Ts...>::type;

template <>
struct contravariance<>
{
};
template <typename T>
struct contravariance<T>
{
    using type = T;
};
template <typename T, typename... Ts>
struct contravariance<T, T, Ts...> : contravariance<T, Ts...>
{
};
template <typename T1, typename T2, typename... Ts>
    requires std::is_base_of_v<T1, T2>
struct contravariance<T1, T2, Ts...> : contravariance<T2, Ts...>
{
};
template <typename T1, typename T2, typename... Ts>
    requires std::is_base_of_v<T2, T1>
struct contravariance<T1, T2, Ts...> : contravariance<T1, Ts...>
{
};
template <typename... Ts>
struct contravariance<void, Ts...> : contravariance<Ts...>
{
};
template <typename T, typename... Ts>
struct contravariance<T, void, Ts...> : contravariance<T, Ts...>
{
};

template <typename T>
struct is_type_identity : std::false_type
{
};
template <typename T>
struct is_type_identity<std::type_identity<T>> : std::true_type
{
};

template <typename T>
using remove_cref_t = std::remove_const_t<std::remove_reference_t<T>>;

} // namespace dplx::dp::detail
