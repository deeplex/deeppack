
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/copy_item.hpp"

#include <cstddef>
#include <new>

#include <boost/container/small_vector.hpp>

#include <dplx/dp/items/emit_core.hpp>
#include <dplx/dp/items/parse_context.hpp>
#include <dplx/dp/items/parse_core.hpp>
#include <dplx/dp/streams/output_buffer.hpp>

namespace dplx::dp
{

namespace detail
{

DPLX_ATTR_FORCE_INLINE static auto
small_buffer_copy(input_buffer &in,
                  std::size_t amount,
                  output_buffer &out) noexcept -> result<void>
{
    DPLX_TRY(out.ensure_size(amount));
    DPLX_TRY(in.bulk_read(out.data(), amount));
    out.commit_written(amount);
    return outcome::success();
}
DPLX_ATTR_FORCE_INLINE static auto
bulk_copy(input_buffer &in, std::uint64_t amount, output_buffer &out) noexcept
        -> result<void>
{
    do
    {
        // dance around gcc's useless cast warning 🤬
#if SIZE_MAX == UINT64_MAX
        std::size_t chunk(amount);
#else
        std::size_t chunk = amount > SIZE_MAX
                                    ? SIZE_MAX
                                    : static_cast<std::size_t>(amount);
#endif
        DPLX_TRY(out.ensure_size(chunk));
        chunk = std::min(chunk, out.size());
        DPLX_TRY(in.bulk_read(out.data(), chunk));
        out.commit_written(chunk);
        amount -= chunk;
    }
    while (amount > 0U);
    return outcome::success();
}

DPLX_ATTR_FORCE_INLINE static auto copy_special_break_to(parse_context &ctx,
                                                         output_buffer &out)
        -> result<void>
{

    ctx.in.discard_buffered(1U);
    DPLX_TRY(out.ensure_size(1U));
    *out.data() = static_cast<std::byte>(type_code::special_break);
    out.commit_written(1U);
    return outcome::success();
}

static auto copy_binary_or_text_to(parse_context &ctx,
                                   item_head const &item,
                                   output_buffer &out) -> result<void>
{
    if (!item.indefinite()) [[likely]]
    {
        return detail::bulk_copy(ctx.in, item.value, out);
    }

    for (;;)
    {
        item_head chunkInfo; // NOLINT(cppcoreguidelines-pro-type-member-init)
        if (auto &&parseHeadRx = dp::peek_item_head(ctx);
            parseHeadRx.has_value()) [[likely]]
        {
            chunkInfo = parseHeadRx.assume_value();
        }
        else
        {
            return static_cast<decltype(parseHeadRx) &&>(parseHeadRx)
                    .assume_error();
        }

        if (chunkInfo.is_special_break())
        {
            DPLX_TRY(detail::copy_special_break_to(ctx, out));
            break;
        }
        if (chunkInfo.type != item.type || chunkInfo.indefinite())
        {
            return errc::invalid_indefinite_subitem;
        }

        DPLX_TRY(detail::bulk_copy(
                ctx.in, chunkInfo.encoded_length + chunkInfo.value, out));
    }
    return outcome::success();
}

} // namespace detail

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto copy_item_to(parse_context &ctx, output_buffer &out) noexcept
        -> result<void>
{
    // note that for every valid item head the following holds:
    // encoded_length < minimum_output_buffer_size

    constexpr int majorTypeBitOffset = 5;
    constexpr std::size_t numStackItems = 64;

    boost::container::small_vector<item_head, numStackItems> stack;
    auto pushItemHead = [&ctx, &out, &stack](item_head head) -> result<void> {
        try
        {
            DPLX_TRY(detail::small_buffer_copy(ctx.in, head.encoded_length,
                                               out));
            stack.push_back(head);
            return outcome::success();
        }
        catch (std::bad_alloc const &)
        {
            return errc::not_enough_memory;
        }
    };

    if (auto &&parseHeadRx = dp::peek_item_head(ctx); parseHeadRx.has_error())
    {
        return static_cast<decltype(parseHeadRx) &&>(parseHeadRx)
                .assume_error();
    }
    else // NOLINT(readability-else-after-return)
    {
        DPLX_TRY(pushItemHead(parseHeadRx.assume_value()));
    }

    do
    {
        auto &item = stack.back();
        // NOLINTNEXTLINE(bugprone-switch-missing-default-case)
        switch (static_cast<std::uint8_t>(item.type) >> majorTypeBitOffset)
        {
        case static_cast<unsigned>(type_code::special) >> majorTypeBitOffset:
            if (item.indefinite())
            {
                // special break has no business being here.
                return errc::item_type_mismatch;
            }
            [[fallthrough]];
        case static_cast<unsigned>(type_code::posint) >> majorTypeBitOffset:
        case static_cast<unsigned>(type_code::negint) >> majorTypeBitOffset:
            stack.pop_back();
            break;

        case static_cast<unsigned>(type_code::binary) >> majorTypeBitOffset:
        case static_cast<unsigned>(type_code::text) >> majorTypeBitOffset: {
            // neither finite nor indefinite binary/text items can be nested
            DPLX_TRY(detail::copy_binary_or_text_to(ctx, item, out));
            stack.pop_back();
            break;
        }

        case static_cast<unsigned>(type_code::array) >> majorTypeBitOffset: {
            if (item.value == 0)
            {
                stack.pop_back();
                break;
            }

            bool const indefinite = item.indefinite();
            // for indefinite arrays we keep item.value safely at 0x1f != 0
            item.value -= static_cast<unsigned>(!indefinite);

            // item reference can be invalidated by push back
            DPLX_TRY(item_head subItem, dp::peek_item_head(ctx));
            if (!indefinite || !subItem.is_special_break()) [[likely]]
            {
                DPLX_TRY(pushItemHead(subItem));
            }
            else [[unlikely]]
            {
                // special break => it's over
                DPLX_TRY(detail::copy_special_break_to(ctx, out));
                stack.pop_back();
            }
            break;
        }

        case static_cast<unsigned>(type_code::map) >> majorTypeBitOffset: {
            if (item.value == 0)
            {
                stack.pop_back();
                break;
            }

            auto const rawFlags = static_cast<unsigned>(item.flags);
            auto const indefinite
                    = (rawFlags
                       & static_cast<unsigned>(item_head::flag::indefinite))
                      != 0U;
            // we abuse item.flags to track whether to expect a value or key
            // code == 0b0x  =>  next item is a key
            // code == 0b1x  =>  next item is a value, therefore decrement kv
            // ctr
            auto const decr = (rawFlags >> 1);
            item.flags = static_cast<item_head::flag>(rawFlags ^ 2U);

            // for indefinite maps we keep item.value safely at 0x1f != 0
            item.value -= decr & static_cast<unsigned>(!indefinite);

            // item reference can be invalidated by push back
            DPLX_TRY(item_head subItem, dp::peek_item_head(ctx));
            if (!indefinite || !subItem.is_special_break()) [[likely]]
            {
                DPLX_TRY(pushItemHead(subItem));
            }
            else [[unlikely]]
            {
                if (decr != 0U)
                {
                    // uhhh, an odd number of items in a map
                    return errc::item_type_mismatch;
                }
                // special break => it's over
                DPLX_TRY(detail::copy_special_break_to(ctx, out));
                stack.pop_back();
            }

            break;
        }

        case static_cast<unsigned>(type_code::tag) >> majorTypeBitOffset: {
            DPLX_TRY(item, dp::peek_item_head(ctx));
            DPLX_TRY(detail::small_buffer_copy(ctx.in, item.encoded_length,
                                               out));
            break;
        }
        }
    }
    while (!stack.empty());

    return outcome::success();
}

} // namespace dplx::dp
