
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/std-string.hpp"

#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <dplx/dp/api.hpp>

#include "dplx/dp/streams/memory_output_stream2.hpp"
#include "item_sample.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

// These APIs just dispatch to `dp::emit_u8string(…)`.
// Therefore we only test API conformance.
// Actual string emitter tests can be found in emit_core.test.cpp

TEST_CASE("std::u8string_view should be encodable")
{
    item_sample<std::u8string_view> const sample{
            u8"some", 5, {0x64, u8's', u8'o', u8'm', u8'e'}
    };

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);

    REQUIRE(dp::encode(outputStream, sample.value));

    CHECK(std::ranges::equal(buffer, sample.encoded_bytes()));
}

TEST_CASE("std::u8string should be encodable")
{
    item_sample<std::u8string> const sample{
            u8"some", 5, {0x64, u8's', u8'o', u8'm', u8'e'}
    };

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);

    REQUIRE(dp::encode(outputStream, sample.value));

    CHECK(std::ranges::equal(buffer, sample.encoded_bytes()));
}

TEST_CASE("std::string_view should be encodable")
{
    item_sample<std::string_view> const sample{
            "some", 5, {0x64, u8's', u8'o', u8'm', u8'e'}
    };

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);

    REQUIRE(dp::encode(outputStream, sample.value));

    CHECK(std::ranges::equal(buffer, sample.encoded_bytes()));
}

TEST_CASE("std::string should be encodable")
{
    item_sample<std::string> const sample{
            "some", 5, {0x64, u8's', u8'o', u8'm', u8'e'}
    };

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);

    REQUIRE(dp::encode(outputStream, sample.value));

    CHECK(std::ranges::equal(buffer, sample.encoded_bytes()));
}

} // namespace dp_tests
