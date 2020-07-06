
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

#include <dplx/dp/detail/mp_lite.hpp>

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
concept integer = std::integral<T> &&detail::none_of<std::remove_cv_t<T>,
                                                     bool,
                                                     char,
                                                     wchar_t,
                                                     char8_t,
                                                     char16_t,
                                                     char32_t>;

// clang-format off
template <typename T>
concept iec559_floating_point
    = std::is_floating_point_v<T>
    && (sizeof(T) == 4 || sizeof(T) == 8)
    && requires
    {
        typename std::numeric_limits<T>;

        // MSVC concepts implementation bug | recheck every version.
        // e.g. iec559_floating_point<int[2]> causes a
        // C2090: function returns array
        // compilation error in the fallback std::numeric_limits<T>
        // implementation.
#if !defined(_MSC_VER) || _MSC_VER > 1927 || defined(__clang__)
        requires std::numeric_limits<T>::is_iec559;
#endif
    };
// clang-format on

// clang-format off
template <typename T>
concept pair_like
    = requires(T &&t)
    {
        typename std::tuple_size<T>::type;
        requires std::derived_from<std::tuple_size<T>, std::integral_constant<std::size_t, 2>>;
        typename std::tuple_element<0, T>::type;
        typename std::tuple_element<1, T>::type;
        { get<0>(t) } -> std::convertible_to<std::tuple_element_t<0, T> &>;
        { get<1>(t) } -> std::convertible_to<std::tuple_element_t<1, T> &>;
    };
// clang-format on

} // namespace dplx::dp

namespace dplx::dp::detail
{

template <typename T>
concept tuple_sized = requires
{
    typename std::tuple_size<T>::type;
};

template <typename F, typename T, std::size_t... Is>
constexpr decltype(auto)
apply_simply_impl(F &&f, T &&t, std::index_sequence<Is...>) noexcept(
    noexcept(std::forward<F>(f)(get<Is>(std::forward<T>(t))...)))
{
    return std::forward<F>(f)(get<Is>(std::forward<T>(t))...);
}
// a poor man's std::apply() which however uses unqualified get<I>()
// instead of std::get<I>(). This allows it to cope with custom tuple types.
template <typename F, typename T>
requires tuple_sized<std::remove_cvref_t<T>> constexpr decltype(auto)
apply_simply(F &&f, T &&t) noexcept(noexcept(dp::detail::apply_simply_impl(
    std::forward<F>(f),
    std::forward<T>(t),
    std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<T>>>())))
{
    return ::dplx::dp::detail::apply_simply_impl<F, T>(
        std::forward<F>(f),
        std::forward<T>(t),
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

} // namespace dplx::dp::detail
