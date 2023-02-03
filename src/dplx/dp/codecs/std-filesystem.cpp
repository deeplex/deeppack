
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/std-filesystem.hpp"

#include <new>

#include <dplx/dp/items/emit_core.hpp>
#include <dplx/dp/items/parse_ranges.hpp>

namespace dplx::dp
{
auto codec<std::filesystem::path>::size_of(
        emit_context &ctx, std::filesystem::path const &path) noexcept
        -> std::uint64_t
try
{
    auto const generic = path.generic_u8string();
    return dp::item_size_of_u8string(ctx, generic.size());
}
catch (...)
{
    return 0U;
}
auto codec<std::filesystem::path>::encode(
        emit_context &ctx, std::filesystem::path const &path) noexcept
        -> result<void>
try
{
    auto const generic = path.generic_u8string();

    return dp::emit_u8string(ctx, generic.data(), generic.size());
}
catch (std::bad_alloc const &)
{
    return dp::errc::not_enough_memory;
}
catch (...)
{
    return dp::errc::bad;
}
auto codec<std::filesystem::path>::decode(parse_context &ctx,
                                          std::filesystem::path &path) noexcept
        -> result<void>
try
{
    std::u8string generic;
    DPLX_TRY(dp::parse_text(ctx, generic));
    path.assign(static_cast<std::u8string &&>(generic));
    return dp::success();
}
catch (std::bad_alloc const &)
{
    return dp::errc::not_enough_memory;
}
catch (...)
{
    return dp::errc::bad;
}
} // namespace dplx::dp
