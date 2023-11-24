
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/skip_item.hpp"

#include <cstddef>
#include <new>

#include <boost/container/small_vector.hpp>

#include <dplx/dp/items/parse_context.hpp>
#include <dplx/dp/items/parse_core.hpp>

namespace dplx::dp
{

namespace detail
{

static auto skip_binary_or_text(parse_context &ctx, item_head const &item)
        -> result<void>
{
    if (!item.indefinite()) [[likely]]
    {
        return ctx.in.discard_input(item.value);
    }

    for (;;)
    {
        item_head chunkInfo; // NOLINT(cppcoreguidelines-pro-type-member-init)
        if (auto &&parseHeadRx = dp::parse_item_head(ctx);
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
            break;
        }
        if (chunkInfo.type != item.type || chunkInfo.indefinite())
        {
            return errc::invalid_indefinite_subitem;
        }

        DPLX_TRY(ctx.in.discard_input(chunkInfo.value));
    }
    return outcome::success();
}

} // namespace detail

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto skip_item(parse_context &ctx) noexcept -> result<void>
{
    constexpr int majorTypeBitOffset = 5;
    constexpr std::size_t numStackItems = 64;

    boost::container::small_vector<item_head, numStackItems> stack;
    if (auto &&parseHeadRx = dp::parse_item_head(ctx); parseHeadRx.has_error())
    {
        return static_cast<decltype(parseHeadRx) &&>(parseHeadRx)
                .assume_error();
    }
    else // NOLINT(readability-else-after-return)
    {
        stack.emplace_back() = parseHeadRx.assume_value();
    }

    do
    {
        auto &item = stack.back();
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
            if (auto &&skipBinaryOrTextRx
                = detail::skip_binary_or_text(ctx, item);
                skipBinaryOrTextRx.has_error())
            {
                return static_cast<decltype(skipBinaryOrTextRx) &&>(
                               skipBinaryOrTextRx)
                        .assume_error();
            }
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
            DPLX_TRY(item_head subItem, dp::parse_item_head(ctx));
            if (!indefinite || !subItem.is_special_break()) [[likely]]
            {
                try
                {
                    stack.push_back(subItem);
                }
                catch (std::bad_alloc const &)
                {
                    return errc::not_enough_memory;
                }
            }
            else [[unlikely]]
            {
                // special break => it's over
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
            DPLX_TRY(item_head subItem, dp::parse_item_head(ctx));
            if (!indefinite || !subItem.is_special_break()) [[likely]]
            {
                try
                {
                    stack.push_back(subItem);
                }
                catch (std::bad_alloc const &)
                {
                    return errc::not_enough_memory;
                }
            }
            else [[unlikely]]
            {
                if (decr != 0U)
                {
                    // uhhh, an odd number of items in a map
                    return errc::item_type_mismatch;
                }
                // special break => it's over
                stack.pop_back();
            }

            break;
        }

        case static_cast<unsigned>(type_code::tag) >> majorTypeBitOffset: {
            DPLX_TRY(item, dp::parse_item_head(ctx));
            break;
        }
        }
    }
    while (!stack.empty());

    return outcome::success();
}

} // namespace dplx::dp
