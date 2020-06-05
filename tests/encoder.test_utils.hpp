
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/dp/encoder/core.hpp>

#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{
struct simple_encodeable
{
    std::byte value;
};
}

namespace dplx::dp
{
template <typename Stream>
class basic_encoder<Stream, dp_tests::simple_encodeable>
{
    Stream *mOutStream;

public:
    explicit basic_encoder(Stream &outStream)
        : mOutStream(&outStream)
    {
    }

    auto operator()(dp_tests::simple_encodeable x) -> result<void>
    {
        DPLX_TRY(writeLease, dplx::dp::write(*mOutStream, 1));
        std::ranges::data(writeLease)[0] = x.value;
        return success();
    }
};
} // namespace dplx::dp
