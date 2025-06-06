
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/legacy/memory_input_stream.hpp"

#include <cstddef>

#include <catch2/catch_test_macros.hpp>
#include <fmt/ranges.h>

#include <dplx/dp/api.hpp>
#include <dplx/dp/codecs/core.hpp>

#include "item_sample_ct.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

static_assert(dp::input_stream<dp::memory_view &>);

TEST_CASE("legacy memory_view can be read from")
{
    auto const sample = item_sample_ct<int>{
            0x7e, 2, {0x18, 0x7e}
    };

    dp::memory_view subject{sample.encoded_bytes()};

    int value = -1;
    REQUIRE(dp::decode(subject, value));

    CHECK(value == sample.value);
    CHECK(subject.consumed_size() == sample.encoded_length);
}

} // namespace dp_tests
