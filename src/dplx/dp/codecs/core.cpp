
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/core.hpp"

#include <dplx/dp/items/emit_core.hpp>

namespace dplx::dp
{

auto codec<null_type>::encode(emit_context const &ctx, null_type) noexcept
        -> result<void>
{
    return dp::emit_null(ctx);
}

auto codec<signed char>::encode(emit_context const &ctx,
                                signed char value) noexcept -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<unsigned char>::encode(emit_context const &ctx,
                                  unsigned char value) noexcept -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<short>::encode(emit_context const &ctx, short value) noexcept
        -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<unsigned short>::encode(emit_context const &ctx,
                                   unsigned short value) noexcept
        -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<int>::encode(emit_context const &ctx, int value) noexcept
        -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<unsigned>::encode(emit_context const &ctx, unsigned value) noexcept
        -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<long>::encode(emit_context const &ctx, long value) noexcept
        -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<unsigned long>::encode(emit_context const &ctx,
                                  unsigned long value) noexcept -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<long long>::encode(emit_context const &ctx, long long value) noexcept
        -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<unsigned long long>::encode(emit_context const &ctx,
                                       unsigned long long value) noexcept
        -> result<void>
{
    return dp::emit_integer(ctx, value);
}

auto codec<bool>::encode(emit_context const &ctx, bool value) noexcept
        -> result<void>
{
    return dp::emit_boolean(ctx, value);
}

auto codec<float>::encode(emit_context const &ctx, float value) noexcept
        -> result<void>
{
    return dp::emit_float_single(ctx, value);
}

auto codec<double>::encode(emit_context const &ctx, double value) noexcept
        -> result<void>
{
    return dp::emit_float_double(ctx, value);
}

} // namespace dplx::dp
