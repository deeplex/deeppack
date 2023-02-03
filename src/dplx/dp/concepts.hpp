
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <climits>
#include <concepts>
#include <type_traits>
#include <utility>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>

static_assert(CHAR_BIT == 8); // NOLINT(cppcoreguidelines-avoid-magic-numbers)

namespace dplx::dp
{

// clang-format off
template <typename T>
concept encodable
    = requires(T const t, emit_context ctx)
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

// clang-format off
template <typename T>
concept value_decodable
    = decodable<T>
    && requires { typename T; }
    && ((std::default_initializable<T> && std::movable<T>)
        || requires(parse_context ctx)
           {
               { codec<T>::decode(ctx) } noexcept
                   -> detail::tryable_result<T>;
           });
// clang-format on

// clang-format off
template <typename T>
concept codable
    = decodable<T>
    || encodable<T>;
// clang-format on

} // namespace dplx::dp

namespace dplx::dp
{

template <typename Enum>
inline constexpr bool disable_enum_codec = false;
template <>
inline constexpr bool disable_enum_codec<std::byte> = true;

template <typename Enum>
concept codable_enum = std::is_enum_v<Enum> && (!disable_enum_codec<Enum>);

} // namespace dplx::dp
