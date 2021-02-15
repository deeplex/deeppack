
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <new>
#include <utility>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/item_parser.hpp>
#include <dplx/dp/stream.hpp>

#include <boost/predef/compiler.h>
#include <boost/predef/other/workaround.h>

namespace dplx::dp
{

#if BOOST_PREDEF_WORKAROUND(BOOST_COMP_MSVC, <, 19, 29, 0)
namespace detail
{

template <typename T>
concept resizable_container = requires(T &&value, std::size_t const numElements)
{
    value.reserve(numElements);
};

} // namespace detail
#endif

template <typename T, input_stream Stream, typename DecodeElementFn>
auto parse_array(Stream &stream,
                 T &value,
                 type_code expectedItemType,
                 DecodeElementFn &&decodeElement) -> result<void>
    // clang-format off
requires requires
{
    { decodeElement(stream, value) }
        -> oc::concepts::value_or_error;
}
// clang-format on
try
{
    DPLX_TRY(auto &&arrayInfo, detail::parse_item_info(stream));

    if (static_cast<std::byte>(arrayInfo.type & 0b111'00000) !=
        expectedItemType)
    {
        return errc::item_type_mismatch;
    }

    if (!arrayInfo.indefinite())
    {
        DPLX_TRY(std::size_t remainingInputSize,
                 dp::available_input_size(stream));

        remainingInputSize >>= (expectedItemType == type_code::map ? 1 : 0);
        if (arrayInfo.value > remainingInputSize)
        {
            return errc::missing_data;
        }
        if (!std::in_range<std::size_t>(arrayInfo.value))
        {
            return errc::not_enough_memory;
        }

        auto const numElements = static_cast<std::size_t>(arrayInfo.value);
#if BOOST_PREDEF_WORKAROUND(BOOST_COMP_MSVC, <, 19, 29, 0)
        if constexpr (detail::resizable_container<T>)
#else
        if constexpr (requires { value.reserve(numElements); })
#endif
        {
            value.reserve(numElements);
        }

        for (std::size_t i = 0; i < numElements; ++i)
        {
            DPLX_TRY(decodeElement(stream, value));
        }
        return oc::success();
    }
    else
    {
        for (;;)
        {
            {
                DPLX_TRY(auto &&maybeStop, dp::read(stream, 1));
                if (std::ranges::data(maybeStop)[0] == type_code::special_break)
                {
                    if constexpr (lazy_input_stream<Stream>)
                    {
                        DPLX_TRY(dp::consume(stream, maybeStop));
                    }
                    break;
                }

                DPLX_TRY(dp::consume(stream, maybeStop, 0));
            }

            DPLX_TRY(decodeElement(stream, value));
        }
        return oc::success();
    }
}
catch (std::bad_alloc const &)
{
    return errc::not_enough_memory;
}

} // namespace dplx::dp
