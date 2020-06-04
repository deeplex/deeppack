
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

#include <dplx/dp/tag_invoke.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/utils.hpp>

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
using select_proper_param_type =
    std::conditional_t<enable_pass_by_value<std::remove_cvref_t<T>>,
                       std::remove_reference_t<T>,
                       T &> const;

template <typename... Ts>
struct mp_list
{
};

template <typename T, template <typename...> typename U>
struct mp_rename
{
};
template <template <typename...> typename T,
          template <typename...>
          typename U,
          typename... Ts>
struct mp_rename<T<Ts...>, U>
{
    using type = U<Ts...>;
};
template <typename T, template <typename...> typename U>
using mp_rename_t = typename mp_rename<T, U>::type;

template <typename T>
concept tuple_sized = requires
{
    typename std::tuple_size<T>::type;
};

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

template <output_stream Stream, typename T>
struct are_tuple_elements_encodeable : std::false_type
{
};
template <output_stream Stream, typename... Ts>
struct are_tuple_elements_encodeable<Stream, mp_list<Ts...>>
    : std::bool_constant<(encodeable<Stream, Ts> && ...)>
{
};

template <typename Stream, typename T>
concept encodeable_tuple_like = tuple_like<T> &&output_stream<Stream>
    &&are_tuple_elements_encodeable<Stream, tuple_element_list_t<T>>::value;

} // namespace dplx::dp::detail
