
// Copyright Henrik Steffen Ga├ƒmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cassert>
#include <cstdint>
#include <ranges>

#include <dplx/dp/api.hpp>
#include <dplx/dp/concepts.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/encoded_item_head_size.hpp>

namespace dplx::dp
{

template <std::ranges::input_range R>
    // clang-format off
    requires((std::ranges::forward_range<R> || std::ranges::sized_range<R>)
            && ng::encodable<std::ranges::range_value_t<R>>)
// clang-format on
inline auto item_size_of_array(emit_context const &ctx, R const &vs) noexcept
        -> std::uint64_t
{
    if constexpr (std::ranges::sized_range<R>)
    {
        detail::encodable_int auto const numElements = std::ranges::size(vs);
        std::uint64_t size
                = dp::encoded_item_head_size<type_code::array>(numElements);

        for (auto &&v : vs)
        {
            size += encoded_size_of(ctx, v);
        }
        return size;
    }
    else
    {
        auto it = std::ranges::begin(vs);
        auto const end = std::ranges::end(vs);
        detail::encodable_int auto const numElements
                = std::ranges::distance(it, end);

        std::uint64_t size
                = dp::encoded_item_head_size<type_code::array>(numElements);

        for (; it != end; ++it)
        {
            size += encoded_size_of(ctx, *it);
        }
        return size;
    }
}
template <std::ranges::input_range R>
    requires(ng::encodable<std::ranges::range_value_t<R>>)
inline auto item_size_of_array_indefinite(emit_context const &ctx,
                                          R const &vs) noexcept -> std::uint64_t
{
    // begin and special break
    std::uint64_t size = 1U + 1U;

    for (auto &&v : vs)
    {
        size += encoded_size_of(ctx, v);
    }
    return size;
}

template <std::ranges::input_range R>
    // clang-format off
    requires((std::ranges::forward_range<R> || std::ranges::sized_range<R>)
            && detail::encodable_pair_like2<std::ranges::range_value_t<R>>)
// clang-format on
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
inline auto item_size_of_map(emit_context const &ctx, R const &vs) noexcept
        -> std::uint64_t
{
    if constexpr (std::ranges::sized_range<R>)
    {
        detail::encodable_int auto const numElements = std::ranges::size(vs);
        std::uint64_t size
                = dp::encoded_item_head_size<type_code::map>(numElements);

        for (auto &&[first, second] : vs)
        {
            size += encoded_size_of(ctx, first);
            size += encoded_size_of(ctx, second);
        }
        return size;
    }
    else
    {
        auto it = std::ranges::begin(vs);
        auto const end = std::ranges::end(vs);
        detail::encodable_int auto const numElements
                = std::ranges::distance(it, end);

        std::uint64_t size
                = dp::encoded_item_head_size<type_code::map>(numElements);

        for (; it != end; ++it)
        {
            auto &&[first, second] = *it;
            size += encoded_size_of(ctx, first);
            size += encoded_size_of(ctx, second);
        }
        return size;
    }
}
template <std::ranges::input_range R>
    requires(detail::encodable_pair_like2<std::ranges::range_value_t<R>>)
inline auto item_size_of_map_indefinite(emit_context const &ctx,
                                        R const &vs) noexcept -> std::uint64_t
{
    // begin and special break
    std::uint64_t size = 1U + 1U;

    for (auto &&[first, second] : vs)
    {
        size += encoded_size_of(ctx, first);
        size += encoded_size_of(ctx, second);
    }
    return size;
}

} // namespace dplx::dp
