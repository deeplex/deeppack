
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <concepts>
#include <cstddef>
#include <ranges>
#include <span>

#include <dplx/dp/api.hpp>
#include <dplx/dp/concepts.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/emit_core.hpp>
#include <dplx/dp/streams/output_buffer.hpp>

namespace dplx::dp
{

template <std::ranges::input_range R>
    // clang-format off
    requires((std::ranges::forward_range<R> || std::ranges::sized_range<R>)
            && ng::encodable<std::ranges::range_value_t<R>>)
// clang-format on
inline auto emit_array(emit_context const &ctx, R const &vs) noexcept
        -> result<void>
{
    if constexpr (std::ranges::sized_range<R>)
    {
        detail::encodable_int auto const size = std::ranges::size(vs);
        DPLX_TRY(dp::emit_array(ctx, size));

        for (auto &&v : vs)
        {
            DPLX_TRY(encode(ctx, v));
        }
        return oc::success();
    }
    else
    {
        auto it = std::ranges::begin(vs);
        auto const end = std::ranges::end(vs);
        detail::encodable_int auto const size = std::ranges::distance(it, end);

        DPLX_TRY(dp::emit_array(ctx, size));

        for (; it != end; ++it)
        {
            DPLX_TRY(encode(ctx, *it));
        }
        return oc::success();
    }
}
template <std::ranges::input_range R>
    requires(ng::encodable<std::ranges::range_value_t<R>>)
inline auto emit_array_indefinite(emit_context const &ctx, R const &vs) noexcept
        -> result<void>
{
    DPLX_TRY(dp::emit_array_indefinite(ctx));
    for (auto &&v : vs)
    {
        DPLX_TRY(encode(ctx, v));
    }
    return dp::emit_break(ctx);
}

template <std::ranges::input_range R>
    // clang-format off
    requires((std::ranges::forward_range<R> || std::ranges::sized_range<R>)
            && detail::encodable_pair_like2<std::ranges::range_value_t<R>>)
// clang-format on
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
inline auto emit_map(emit_context const &ctx, R const &vs) noexcept
        -> result<void>
{
    if constexpr (std::ranges::sized_range<R>)
    {
        detail::encodable_int auto const size = std::ranges::size(vs);
        DPLX_TRY(dp::emit_map(ctx, size));

        for (auto &&[first, second] : vs)
        {
            DPLX_TRY(encode(ctx, first));
            DPLX_TRY(encode(ctx, second));
        }
        return oc::success();
    }
    else
    {
        auto it = std::ranges::begin(vs);
        auto const end = std::ranges::end(vs);
        detail::encodable_int auto const size = std::ranges::distance(it, end);

        DPLX_TRY(dp::emit_map(ctx, size));

        for (; it != end; ++it)
        {
            auto &&[first, second] = *it;
            DPLX_TRY(encode(ctx, first));
            DPLX_TRY(encode(ctx, second));
        }
        return oc::success();
    }
}
template <std::ranges::input_range R>
    requires(detail::encodable_pair_like2<std::ranges::range_value_t<R>>)
inline auto emit_map_indefinite(emit_context const &ctx, R const &vs) noexcept
        -> result<void>
{
    DPLX_TRY(dp::emit_map_indefinite(ctx));
    for (auto &&[first, second] : vs)
    {
        DPLX_TRY(encode(ctx, first));
        DPLX_TRY(encode(ctx, second));
    }
    return dp::emit_break(ctx);
}

} // namespace dplx::dp
