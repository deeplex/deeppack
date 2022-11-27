
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <ranges>

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/emit_core.hpp>
#include <dplx/dp/items/emit_ranges.hpp>

namespace dplx::dp
{

template <range R>
    requires std::ranges::contiguous_range<
            R> && std::same_as<std::byte, std::ranges::range_value_t<R>>
class codec<R>
{
public:
    static auto encode(emit_context const &ctx,
                       std::span<std::byte const> value) noexcept
            -> result<void>
    {
        return dp::emit_binary(ctx, std::ranges::data(value),
                               std::ranges::size(value));
    }
};

template <range R>
class codec<R>
{
public:
    static auto encode(emit_context const &ctx, R const &vs) noexcept
            -> result<void>
        requires std::ranges::input_range<R>
        {
            if constexpr (enable_indefinite_encoding<R>)
            {
                return dp::emit_array_indefinite(ctx, vs);
            }
            else
            {
                return dp::emit_array(ctx, vs);
            }
        }
};

template <associative_range R>
class codec<R>
{
public:
    static auto encode(emit_context const &ctx, R const &vs) noexcept
            -> result<void>
        requires std::ranges::input_range<R>
        {
            if constexpr (enable_indefinite_encoding<R>)
            {
                return dp::emit_map_indefinite(ctx, vs);
            }
            else
            {
                return dp::emit_map(ctx, vs);
            }
        }
};

} // namespace dplx::dp
