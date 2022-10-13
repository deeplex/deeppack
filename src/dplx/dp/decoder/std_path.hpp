
// Copyright Henrik Steffen Gaßmann 2021.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <filesystem>
#include <string>

#include <dplx/dp/fwd.hpp>
#include <dplx/dp/item_parser.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp
{

template <input_stream Stream>
class basic_decoder<std::filesystem::path, Stream>
{
    using parse = item_parser<Stream>;

public:
    using value_type = std::filesystem::path;

    auto operator()(Stream &inStream, value_type &value) -> result<void>
    {
        std::u8string u8Representation;
        DPLX_TRY(parse::u8string_finite(inStream, u8Representation));

        try
        {
            value = value_type{static_cast<std::u8string &&>(u8Representation)};
            return oc::success();
        }
        catch (std::bad_alloc const &)
        {
            return errc::not_enough_memory;
        }
    }
};

} // namespace dplx::dp
