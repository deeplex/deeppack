
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
class basic_decoder<std::u8string, Stream>
{
    using parse = item_parser<Stream>;

public:
    auto operator()(Stream &inStream, std::u8string &value) const
            -> result<void>
    {
        DPLX_TRY(parse::u8string(inStream, value));
        return oc::success();
    }
};

} // namespace dplx::dp
