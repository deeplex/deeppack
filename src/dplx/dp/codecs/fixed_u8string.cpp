
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/fixed_u8string.hpp"

#include <span>

#include <dplx/dp/cpos/container.std.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/emit_core.hpp>
#include <dplx/dp/items/item_size_of_core.hpp>
#include <dplx/dp/items/parse_context.hpp>
#include <dplx/dp/items/parse_core.hpp>
#include <dplx/dp/items/parse_ranges.hpp>

namespace dplx::dp::detail
{

auto fixed_u8string_codec_base::size_of(emit_context &ctx,
                                        std::u8string_view value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_u8string(ctx, value.size());
}
auto fixed_u8string_codec_base::encode(emit_context &ctx,
                                       std::u8string_view value) noexcept
        -> result<void>
{
    return dp::emit_u8string(ctx, value.data(), value.size());
}

auto decode_fixed_u8string(parse_context &ctx, char8_t *out, unsigned &outlen)
        -> result<void>
{
    auto const maxlen = std::exchange(outlen, 0U);
    std::span<char8_t> buffer(out, maxlen);
    DPLX_TRY(auto len, dp::parse_text_finite(ctx, buffer, maxlen));

    outlen = static_cast<unsigned>(len);
    return oc::success();
}

} // namespace dplx::dp::detail
