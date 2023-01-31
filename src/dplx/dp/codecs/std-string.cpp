
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/std-string.hpp"

#include <dplx/dp/cpos/container.std.hpp>
#include <dplx/dp/items/emit_core.hpp>
#include <dplx/dp/items/item_size_of_core.hpp>
#include <dplx/dp/items/parse_ranges.hpp>

namespace dplx::dp
{

auto codec<std::u8string_view>::size_of(emit_context const &ctx,
                                        std::u8string_view value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_u8string(ctx, value.size());
}
auto codec<std::u8string_view>::encode(emit_context const &ctx,
                                       std::u8string_view value) noexcept
        -> result<void>
{
    return dp::emit_u8string(ctx, value.data(), value.size());
}

auto codec<std::u8string>::size_of(emit_context const &ctx,
                                   std::u8string const &value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_u8string(ctx, value.size());
}
auto codec<std::u8string>::encode(emit_context const &ctx,
                                  std::u8string const &value) noexcept
        -> result<void>
{
    return dp::emit_u8string(ctx, value.data(), value.size());
}

auto codec<std::u8string>::decode(parse_context &ctx,
                                  std::u8string &value) noexcept -> result<void>
{
    DPLX_TRY(dp::parse_text<std::u8string>(ctx, value));
    return oc::success();
}

auto codec<std::string_view>::size_of(emit_context const &ctx,
                                      std::string_view value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_u8string(ctx, value.size());
}
auto codec<std::string_view>::encode(emit_context const &ctx,
                                     std::string_view value) noexcept
        -> result<void>
{
    return dp::emit_u8string(ctx, value.data(), value.size());
}

auto codec<std::string>::size_of(emit_context const &ctx,
                                 std::string const &value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_u8string(ctx, value.size());
}
auto codec<std::string>::encode(emit_context const &ctx,
                                std::string const &value) noexcept
        -> result<void>
{
    return dp::emit_u8string(ctx, value.data(), value.size());
}

auto codec<std::string>::decode(parse_context &ctx, std::string &value) noexcept
        -> result<void>
{
    DPLX_TRY(dp::parse_text<std::string>(ctx, value));
    return oc::success();
}

} // namespace dplx::dp
