
// Copyright 2023 Henrik Steffen Gaßmann
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/uuid.hpp"

#include <dplx/dp/items.hpp>

namespace dplx::dp
{

auto dplx::dp::codec<cncr::uuid>::decode(parse_context &ctx,
                                         cncr::uuid &value) noexcept
        -> result<void>
{
    constexpr auto stateSize = cncr::uuid::state_size;
    DPLX_TRY(dp::expect_item_head(ctx, type_code::binary, stateSize));

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    cncr::blob<std::byte, stateSize, stateSize> raw;
    DPLX_TRY(ctx.in.bulk_read(static_cast<std::byte *>(raw.values), stateSize));

    value = cncr::uuid(raw.values);
    return outcome::success();
}

auto codec<cncr::uuid>::encode(emit_context &ctx,
                               cncr::uuid const value) noexcept -> result<void>
{
    auto const canonical = value.canonical();
    return dp::emit_binary(ctx,
                           static_cast<std::byte const *>(canonical.values),
                           sizeof(canonical.values));
}

auto codec<cncr::uuid>::size_of(emit_context &ctx, cncr::uuid) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_binary(ctx, cncr::uuid::state_size);
}

} // namespace dplx::dp
