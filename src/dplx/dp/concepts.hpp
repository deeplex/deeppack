
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

#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/detail/utils.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/stream.hpp>

static_assert(CHAR_BIT == 8); // NOLINT(cppcoreguidelines-avoid-magic-numbers)

namespace dplx::dp
{

namespace ng // TODO: unwrap
{

// clang-format off
template <typename T>
concept encodable
    = requires(T const t, emit_context const ctx)
    {
        requires !std::is_reference_v<T>;
        requires !std::is_pointer_v<T>;
        { codec<std::remove_const_t<T>>::encode(ctx, t) } noexcept
            -> oc::concepts::basic_result;
        { codec<std::remove_const_t<T>>::size_of(ctx, t) } noexcept
            -> std::same_as<std::uint64_t>;
    };
// clang-format on

// clang-format off
template <typename T>
concept decodable
    = requires(T t, parse_context ctx)
    {
        requires !std::is_reference_v<T>;
        requires !std::is_pointer_v<T>;
        requires !std::is_const_v<T>;
        { codec<T>::decode(ctx, t) } noexcept
            -> oc::concepts::basic_result;
    };
// clang-format on

} // namespace ng

// clang-format off
template <typename T>
concept codable
    = ng::decodable<T>
    || ng::encodable<T>;
// clang-format on

} // namespace dplx::dp

// legacy
namespace dplx::dp
{

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

} // namespace dplx::dp

namespace dplx::dp
{

template <typename Range>
inline constexpr bool enable_indefinite_encoding
        = std::ranges::input_range<
                  Range> && !std::ranges::sized_range<Range> && !std::ranges::forward_range<Range>;

template <typename T>
inline constexpr bool disable_range
        // ranges which are type recursive w.r.t. their iterator value type
        // (like std::filesystem::path) can't have an encodable base case and
        // therefore always need their own specialization. We exclude them here
        // in order to prevent any partial template specialization ambiguity
        = std::is_same_v<T, std::ranges::range_value_t<T>>;

template <typename T>
concept range = std::ranges::range<T> && !disable_range<T>;

// clang-format off
template <typename C>
concept container
    = range<C>
    && std::ranges::forward_range<C>
    && requires(C const a, C b)
    {
        requires std::regular<C>;
        requires std::destructible<C>;
        requires std::swappable<C>;
        typename C::size_type;
        requires std::unsigned_integral<typename C::size_type>;

        typename C::iterator;
        requires std::same_as<typename C::iterator, std::ranges::iterator_t<C>>;
        typename C::const_iterator;
        requires std::same_as<
            typename C::const_iterator,
            std::ranges::iterator_t<C const>
        >;

        typename C::value_type;
        requires std::destructible<typename C::value_type>;
        requires std::same_as<typename C::value_type, std::ranges::range_value_t<C>>;
        typename C::reference;

        { a.size() }
            -> std::same_as<typename C::size_type>;
        { a.max_size() }
            -> std::same_as<typename C::size_type>;
        { a.empty() }
            -> std::same_as<bool>;

        // this isn't required by the standard -- it excludes the array oddball.
        { b.clear() }
            -> std::same_as<void>;
    };
// clang-format on

// clang-format off
template <typename C>
concept sequence_container
    = container<C>
    && requires(C a)
    {
        { a.emplace_back() }
            -> std::same_as<typename C::reference>;
    };
// clang-format on

// clang-format off
template <typename C>
concept associative_container
    = container<C>
    && requires { typename C::key_type; }
    && requires(C a, typename C::key_type k)
    {
        { a.emplace(static_cast<typename C::key_type &&>(k)) }
            -> std::same_as<std::pair<typename C::iterator, bool>>;
    };
// clang-format on
//
// clang-format off
template <typename C>
concept mapping_associative_container
    = container<C>
    && requires
    {
        typename C::key_type;
        typename C::mapped_type;
    }
    && requires(C a, typename C::key_type k, typename C::mapped_type m)
    {
        { a.emplace(static_cast<typename C::key_type &&>(k),
                    static_cast<typename C::mapped_type &&>(m)) }
            -> std::same_as<std::pair<typename C::iterator, bool>>;
    };
// clang-format on

template <typename Enum>
inline constexpr bool disable_enum_codec = false;
template <>
inline constexpr bool disable_enum_codec<std::byte> = true;

template <typename Enum>
concept codable_enum = std::is_enum_v<Enum> && !disable_enum_codec<Enum>;

} // namespace dplx::dp
