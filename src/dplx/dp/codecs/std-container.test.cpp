
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/std-container.hpp"

#include <array>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "blob_matcher.hpp"
#include "dplx/dp/api.hpp"
#include "dplx/dp/codecs/core.hpp"
#include "dplx/dp/indefinite_range.hpp"
#include "dplx/dp/streams/memory_output_stream2.hpp"
#include "item_sample_ct.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

static_assert(dp::pair_like<std::pair<int, unsigned>>);
static_assert(dp::detail::encodable_pair_like2<std::pair<int, unsigned>>);

TEST_CASE("std::span of bytes has a codec")
{
    item_sample_ct<std::array<unsigned char, 2U>> const sample{
            .value = {0xfe, 0xfe},
            .encoded_length = 3,
            .encoded = {0x42, 0xfe, 0xfe},
    };
    dp::bytes const value = as_bytes(std::span(sample.value));

    SECTION("with encode")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        REQUIRE(dp::encode(outputStream, value));

        CHECK_BLOB_EQ(outputStream.written(), sample.encoded_bytes());
    }
    SECTION("with size_of")
    {
        CHECK(dp::encoded_size_of(value) == sample.encoded_length);
    }
}

TEST_CASE("a range gets encoded")
{
    item_sample_ct<std::array<unsigned char, 2U>> const sample{
            .value = {0xfe, 0xfe},
            .encoded_length = 5,
            .encoded = {0x82, 0x18, 0xfe, 0x18, 0xfe},
    };

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);

    REQUIRE(dp::encode(outputStream, sample.value));

    CHECK(std::ranges::equal(buffer, sample.encoded_bytes()));
}

TEST_CASE("an indefinite range gets encoded")
{
    item_sample_ct<std::array<unsigned char const, 2U>> const sample{
            .value = {0xfe, 0xfe},
            .encoded_length = 6,
            .encoded = {0x9f, 0x18, 0xfe, 0x18, 0xfe, 0xff},
    };

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);

    REQUIRE(dp::encode(outputStream, dp::indefinite_range(sample.value)));

    CHECK(std::ranges::equal(buffer, sample.encoded_bytes()));
}

TEST_CASE("a custom associative range gets encoded")
{
    item_sample_ct<std::array<std::pair<int, unsigned>, 2>> const sample{
            .value = {{{0x04, 0x02}, {0x01, 0x03}}},
            .encoded_length = 5,
            .encoded = {0xa2, 0x04, 0x2, 0x01, 0x03},
    };

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);

    REQUIRE(dp::encode(outputStream, sample.value));

    CHECK(std::ranges::equal(buffer, sample.encoded_bytes()));
}

TEST_CASE("a custom indefinite associative range gets encoded")
{
    item_sample_ct<std::array<std::pair<int, unsigned>, 2>> const sample{
            .value = {{{0x04, 0x02}, {0x01, 0x03}}},
            .encoded_length = 6,
            .encoded = {0xbf, 0x04, 0x2, 0x01, 0x03, 0xff},
    };

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);

    REQUIRE(dp::encode(outputStream, dp::indefinite_range(sample.value)));

    CHECK(std::ranges::equal(buffer, sample.encoded_bytes()));
}

} // namespace dp_tests
