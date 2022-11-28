
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/std-filesystem.hpp"

#include <new>

#include <dplx/dp/items/emit_core.hpp>

namespace dplx::dp
{

auto codec<std::filesystem::path>::encode(
        emit_context const &ctx, std::filesystem::path const &path) noexcept
        -> result<void>
{
    DPLX_TRY(auto const generic,
             [&path]() noexcept -> result<std::u8string>
             {
                 try
                 {
                     return path.generic_u8string();
                 }
                 catch (std::bad_alloc const &)
                 {
                     return dp::errc::not_enough_memory;
                 }
             }());

    return dp::emit_u8string(ctx, generic.data(), generic.size());
}

} // namespace dplx::dp
