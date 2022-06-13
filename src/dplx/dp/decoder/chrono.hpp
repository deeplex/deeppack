
// Copyright Henrik Steffen Gaßmann 2021.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <chrono>

#include <dplx/dp/fwd.hpp>
#include <dplx/dp/item_parser.hpp>

namespace dplx::dp
{

template <typename Rep, typename Period, input_stream Stream>
class basic_decoder<std::chrono::duration<Rep, Period>, Stream>
{
    using parse = item_parser<Stream>;

public:
    using value_type = std::chrono::duration<Rep, Period>;

    auto operator()(Stream &inStream, value_type &value) -> result<void>
    {
        DPLX_TRY(auto count, parse::template integer<Rep>(inStream));
        value = value_type{count};

        return oc::success();
    }
};

} // namespace dplx::dp
