
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <new>

#include <boost/container/small_vector.hpp>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/item_parser.hpp>
#include <dplx/dp/stream.hpp>
#include <dplx/dp/type_code.hpp>

namespace dplx::dp::detail
{

template <input_stream Stream>
inline auto skip_binary_or_text(Stream &inStream, dp::item_info const &item)
        -> result<void>
{
    if (!item.indefinite())
        DPLX_ATTR_LIKELY
        {
            DPLX_TRY(skip_bytes(inStream, item.value));
        }
    else
    {
        for (;;)
        {
            DPLX_TRY(dp::item_info chunkInfo, detail::parse_item(inStream));

            if (chunkInfo.is_special_break())
            {
                break;
            }
            if (chunkInfo.type != item.type || chunkInfo.indefinite())
            {
                return errc::invalid_indefinite_subitem;
            }

            DPLX_TRY(skip_bytes(inStream, chunkInfo.value));
        }
    }
    return oc::success();
}

} // namespace dplx::dp::detail

namespace dplx::dp
{

template <input_stream Stream>
inline auto skip_item(Stream &inStream) -> result<void>
{
    boost::container::small_vector<dp::item_info, 64> stack;
    DPLX_TRY(auto toBeSkipped, detail::parse_item(inStream));
    stack.push_back(toBeSkipped);

    do
    {
        auto &item = stack.back();
        switch (static_cast<std::uint8_t>(to_byte(item.type) >> 5))
        {
        case static_cast<unsigned>(type_code::special) >> 5:
            if (item.indefinite())
            {
                // special break has no business being here.
                return errc::item_type_mismatch;
            }
            [[fallthrough]];
        case static_cast<unsigned>(type_code::posint) >> 5:
        case static_cast<unsigned>(type_code::negint) >> 5:
            stack.pop_back();
            break;

        case static_cast<unsigned>(type_code::binary) >> 5:
        case static_cast<unsigned>(type_code::text) >> 5:
        {
            // neither finite nor indefinite binary/text items can be nested
            DPLX_TRY(detail::skip_binary_or_text(inStream, item));
            stack.pop_back();
            break;
        }

        case static_cast<unsigned>(type_code::array) >> 5:
        {
            bool const indefinite = item.indefinite();
            // for indefinite arrays we keep item.value safely at 0x1f != 0
            item.value -= !indefinite;
            if (item.value == 0)
            {
                stack.pop_back();
            }

            // item reference can be invalidated by push back
            DPLX_TRY(auto subItem, detail::parse_item(inStream));
            if (!indefinite || !subItem.is_special_break())
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
            else
                DPLX_ATTR_UNLIKELY
                {
                    // special break => it's over
                    stack.pop_back();
                }
            break;
        }

        case static_cast<unsigned>(type_code::map) >> 5:
        {
            auto const rawFlags = detail::to_underlying(item.flags);
            unsigned const indefinite
                    = rawFlags
                    & detail::to_underlying(dp::item_info::flag::indefinite);
            // we abuse item.flags to track of whether to expect a value or key
            // code == 0b0x  =>  next item is a key
            // code == 0b1x  =>  next item is a value, therefore decrement kv
            // ctr
            auto const decr = (rawFlags >> 1) & indefinite;
            item.flags = static_cast<dp::item_info::flag>(rawFlags ^ 2u);

            // for indefinite maps we keep item.value safely at 0x1f != 0
            item.value -= decr;
            if (item.value == 0)
            {
                stack.pop_back();
            }

            // item reference can be invalidated by push back
            DPLX_TRY(auto subItem, detail::parse_item(inStream));
            if (!indefinite || !subItem.is_special_break())
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
            else
                DPLX_ATTR_UNLIKELY
                {
                    if (!decr)
                    {
                        // uhhh, an odd number of items in a map
                        return errc::item_type_mismatch;
                    }
                    // special break => it's over
                    stack.pop_back();
                }

            break;
        }

        case static_cast<unsigned>(type_code::tag) >> 5:
        {
            DPLX_TRY(item, detail::parse_item(inStream));
            break;
        }
        }

    } while (!stack.empty());

    return oc::success();
}

} // namespace dplx::dp
