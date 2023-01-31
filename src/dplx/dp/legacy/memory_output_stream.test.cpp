
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/legacy/memory_output_stream.hpp"

#include <cstddef>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <dplx/dp/api.hpp>
#include <dplx/dp/codecs/core.hpp>

#include "item_sample_ct.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

static_assert(dp::output_stream<dp::memory_buffer &>);

TEST_CASE("legacy memory_buffer can be written to")
{
    auto const sample = item_sample_ct<int>{
            0x7e, 2, {0x18, 0x7e}
    };

    std::vector<std::byte> memory(sample.encoded_length);
    dp::memory_buffer subject{std::span<std::byte>(memory)};

    REQUIRE(dp::encode(subject, sample.value));

    CHECK(std::ranges::equal(subject.consumed(), sample.encoded_bytes()));
}

} // namespace dp_tests
