
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

#include <dplx/cncr/mp_lite.hpp>
#include <dplx/cncr/type_utils.hpp>

#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/detail/utils.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/stream.hpp>

static_assert(CHAR_BIT == 8); // NOLINT(cppcoreguidelines-avoid-magic-numbers)

namespace dplx::dp::ng
{

// clang-format off
template <typename T>
concept encodable
    = !std::is_reference_v<T>
    && !std::is_pointer_v<T>
    && requires(T const &t, emit_context const &ctx)
    {
        { codec<std::remove_const_t<T>>::encode(ctx, t) } noexcept
            -> oc::concepts::basic_result;
    };
// clang-format on

// clang-format off
template <typename T>
concept decodable
    = !std::is_reference_v<T>
    && !std::is_pointer_v<T>
    && !std::is_const_v<T>
    && requires(T &t, parse_context &ctx)
    {
        { codec<T>::decode(ctx, t) } noexcept
            -> oc::concepts::basic_result;
    };
// clang-format on

} // namespace dplx::dp::ng

namespace dplx::dp
{

template <typename T>
inline constexpr bool enable_pass_by_value
        = std::is_trivially_copy_constructible_v<T> &&
                  std::is_trivially_copyable_v<T> && sizeof(T) <= 32;

template <typename Range>
inline constexpr bool enable_indefinite_encoding
        = std::ranges::input_range<
                  Range> && !std::ranges::sized_range<Range> && !std::ranges::forward_range<Range>;

template <typename T, input_stream Stream>
class basic_decoder;

// clang-format off
template <typename T, typename Stream>
concept decodable
    = input_stream<Stream>
    && !std::is_reference_v<T>
    && !std::is_pointer_v<T>
    && requires(Stream &inStream, T &dest)
    {
        typename basic_decoder<T, Stream>;
        { basic_decoder<T, Stream>()(inStream, dest) }
            -> oc::concepts::basic_result;
    };
// clang-format on

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

template <typename T>
inline constexpr bool disable_range
        // ranges which are type recursive w.r.t. their iterator value type
        // (like std::filesystem::path) can't have an encodable base case and
        // therefore always need their own specialization. We exclude them here
        // in order to prevent any partial template specialization ambiguity
        = std::is_same_v<T, std::ranges::range_value_t<T>>;

template <typename T>
concept range = std::ranges::range<T> && !disable_range<T>;

template <typename T>
inline constexpr bool disable_associative_range = false;

template <typename T>
concept associative_range = range<T> && pair_like<
        std::ranges::range_value_t<T>> && !disable_associative_range<T>;

template <typename Enum>
inline constexpr bool disable_enum_codec = false;
template <>
inline constexpr bool disable_enum_codec<std::byte> = true;

template <typename Enum>
concept codable_enum = std::is_enum_v<Enum> && !disable_enum_codec<Enum>;

} // namespace dplx::dp

namespace dplx::dp::detail
{

template <typename T>
using select_proper_param_type_impl
        = std::conditional_t<enable_pass_by_value<T>, T const, T const &>;

template <typename T>
using select_proper_param_type
        = select_proper_param_type_impl<cncr::remove_cref_t<T>>;

// clang-format off
template <typename T>
concept encodable_pair_like2
        = pair_like<T>
        && ng::encodable<std::tuple_element_t<0U, T>>
        && ng::encodable<std::tuple_element_t<1U, T>>;
// clang-format on

template <typename T, typename Stream>
concept decodable_pair_like = pair_like<T> && input_stream<Stream> && decodable<
        typename std::tuple_element<0, T>::type,
        Stream> && decodable<typename std::tuple_element<1, T>::type, Stream>;

} // namespace dplx::dp::detail
