
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/std-string.hpp"

#include <dplx/dp/items/emit_core.hpp>

namespace dplx::dp
{

auto codec<std::u8string_view>::encode(emit_context const &ctx,
                                       std::u8string_view value) noexcept
        -> result<void>
{
    return dp::emit_u8string(ctx, value.data(), value.size());
}

auto codec<std::u8string>::encode(emit_context const &ctx,
                                  std::u8string const &value) noexcept
        -> result<void>
{
    return dp::emit_u8string(ctx, value.data(), value.size());
}

auto codec<std::string_view>::encode(emit_context const &ctx,
                                     std::string_view value) noexcept
        -> result<void>
{
    return dp::emit_u8string(ctx, value.data(), value.size());
}

auto codec<std::string>::encode(emit_context const &ctx,
                                std::string const &value) noexcept
        -> result<void>
{
    return dp::emit_u8string(ctx, value.data(), value.size());
}

} // namespace dplx::dp
