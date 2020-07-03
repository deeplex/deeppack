
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

#include <dplx/dp/detail/mp_lite.hpp>
#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/detail/utils.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/stream.hpp>
#include <dplx/dp/tag_invoke.hpp>

static_assert(CHAR_BIT == 8);

namespace dplx::dp
{

template <output_stream Stream, typename T>
class basic_encoder;

template <typename T>
inline constexpr bool
    enable_pass_by_value = std::is_trivially_copy_constructible_v<T> &&
                               std::is_trivially_copyable_v<T> &&
                           sizeof(T) <= 32;

// clang-format off
template <typename Stream, typename T>
concept encodable
    = output_stream<Stream>
    && !std::is_reference_v<T>
    && !std::is_pointer_v<T>
    && requires(Stream &ctx, T const &t)
    {
        typename basic_encoder<Stream, T>;
        basic_encoder<Stream, T>()(ctx, t);
    };
// clang-format on

template <typename Range>
inline constexpr bool enable_indefinite_encoding =
    std::ranges::input_range<Range> && !std::ranges::sized_range<Range> &&
    !std::ranges::forward_range<Range>;

template <input_stream Stream, typename T>
class basic_decoder;

// clang-format off
template <typename Stream, typename T>
concept decodable
    = input_stream<Stream>
    && !std::is_reference_v<T>
    && !std::is_pointer_v<T>
    && std::is_nothrow_default_constructible_v<T>
    && std::is_nothrow_constructible_v<T>
    && std::is_nothrow_assignable_v<T>
    && requires(Stream &stream, T &dest)
    {
        typename basic_decoder<Stream, T>;
        { basic_decoder<Stream, T>()(stream, dest) }
            -> oc::concepts::basic_result;
    };
// clang-format on

template <typename T>
inline constexpr bool disable_associative_range = false;

// clang-format off
template <typename T>
concept associative_range
    = std::ranges::range<T> &&
        pair_like<std::ranges::range_value_t<T>> &&
        !disable_associative_range<T>;
// clang-format on

} // namespace dplx::dp

namespace dplx::dp::detail
{

template <typename T>
using select_proper_param_type_impl =
    std::conditional_t<enable_pass_by_value<T>, T const, T const &>;

template <typename T>
using select_proper_param_type =
    select_proper_param_type_impl<std::remove_cvref_t<T>>;

template <output_stream Stream, typename T>
struct are_tuple_elements_encodeable : std::false_type
{
};
template <output_stream Stream, typename... Ts>
struct are_tuple_elements_encodeable<Stream, mp_list<Ts...>>
    : std::bool_constant<(encodable<Stream, Ts> && ...)>
{
};

template <typename Stream, typename T>
concept encodeable_tuple_like = tuple_like<T> &&output_stream<Stream>
    &&are_tuple_elements_encodeable<Stream, tuple_element_list_t<T>>::value;

} // namespace dplx::dp::detail
