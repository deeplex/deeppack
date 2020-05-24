
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <concepts>
#include <limits>
#include <ranges>
#include <type_traits>

static_assert(CHAR_BIT == 8);

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
concept iec559_floating_point = std::is_floating_point_v<T> &&
    (sizeof(T) == 4 || sizeof(T) == 8);
// clang-format on

// clang-format off
template <typename T>
concept output_stream
    = requires(T &stream, typename T::write_proxy proxy, std::size_t const n)
    {
        typename T::write_proxy;
        requires std::ranges::contiguous_range<typename T::write_proxy>;
        requires std::same_as<
            std::ranges::range_value_t<typename T::write_proxy>,
            std::byte>;
        {stream.write(n)} -> std::same_as<typename T::write_proxy>;
        proxy.shrink(n);
    };
// clang-format on

template <output_stream Stream, typename T>
class basic_encoder;

template <typename T>
inline constexpr bool enable_pass_by_value = std::is_trivially_copyable_v<T> &&
                                             sizeof(T) <= 32;

// clang-format off
template <typename Stream, typename T>
concept encodeable                            
    = output_stream<Stream> &&
        requires(Stream &ctx, T &&t)
        {
            typename basic_encoder<Stream, T>;
            basic_encoder<Stream, T>{ctx}(std::forward<T>(t));
        };
// clang-format on

template <typename Range>
inline constexpr bool enable_indefinite_encoding =
    std::ranges::input_range<Range> && !std::ranges::sized_range<Range> &&
    !std::ranges::forward_range<Range>;

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
using select_proper_param_type =
    std::conditional_t<enable_pass_by_value<std::remove_cvref_t<T>>,
                       std::remove_reference_t<T>,
                       T &> const;

} // namespace dplx::dp::detail
