
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>

#include <dplx/dp/fwd.hpp>
#include <dplx/dp/item_parser.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp
{

template <input_stream Stream>
class basic_decoder<Stream, std::u8string>
{
public:
    auto operator()(Stream &inStream, std::u8string &value) const
        -> result<void>
    {
        DPLX_TRY(strInfo, detail::parse_item_info(inStream));

        if (static_cast<std::byte>(strInfo.type & 0b111'00000) !=
            type_code::text)
        {
            return errc::item_type_mismatch;
        }

        if (!strInfo.indefinite())
            DPLX_ATTR_LIKELY
            {
                DPLX_TRY(availableBytes, dp::available_input_size(inStream));
                if (availableBytes < strInfo.value)
                {
                    return errc::missing_data;
                }

                if (strInfo.value >= std::numeric_limits<std::size_t>::max())
                {
                    return errc::not_enough_memory;
                }
                try
                {
                    value.resize(static_cast<std::size_t>(strInfo.value));
                }
                catch (std::bad_alloc const &)
                {
                    return errc::not_enough_memory;
                }

                DPLX_TRY(dp::read(inStream,
                                  reinterpret_cast<std::byte *>(value.data()),
                                  value.size()));
            }
        else
        {
            value.resize(0);
            for (;;)
            {
                DPLX_TRY(chunkInfo, detail::parse_item_info(inStream));

                if (chunkInfo.type == 0b111'00001)
                {
                    // special break
                    break;
                }
                else if (std::byte{chunkInfo.type} != type_code::text)
                {
                    return errc::invalid_indefinite_subitem;
                }

                DPLX_TRY(availableBytes, dp::available_input_size(inStream));
                if (availableBytes < chunkInfo.value)
                {
                    return errc::missing_data;
                }

                if (chunkInfo.value >= std::numeric_limits<std::size_t>::max())
                {
                    return errc::not_enough_memory;
                }
                auto const currentSize = value.size();
                auto const chunkSize =
                    static_cast<std::size_t>(chunkInfo.value);
                auto const newSize = currentSize + chunkSize;
                if (currentSize > newSize)
                {
                    // overflow
                    return errc::not_enough_memory;
                }

                try
                {
                    value.resize(newSize);
                }
                catch (std::bad_alloc const &)
                {
                    return errc::not_enough_memory;
                }

                DPLX_TRY(dp::read(inStream,
                                  reinterpret_cast<std::byte *>(value.data()) +
                                      currentSize,
                                  chunkSize));
            }
        }
        return success();
    }
};

} // namespace dplx::dp
