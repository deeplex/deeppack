
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/std-tuple.hpp"

#include <catch2/catch_test_macros.hpp>

#include "blob_matcher.hpp"
#include "dplx/dp/api.hpp"
#include "dplx/dp/codecs/core.hpp"
#include "item_sample_ct.hpp"
#include "test_input_stream.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

// NOLINTBEGIN(readability-function-cognitive-complexity)

namespace dp_tests
{

static_assert(dp::encodable<std::tuple<int, long>>);

TEST_CASE("std::tuple has a codec")
{
    constexpr item_sample_ct<std::tuple<int, long long>> sample{
            {   3, 22},
            3, { 0x82,  3,  22}
    };

    SECTION("with encode")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        REQUIRE(dp::encode(outputStream, sample.value));

        CHECK_BLOB_EQ(outputStream.written(), sample.encoded_bytes());
    }
    SECTION("with a size_of operator")
    {
        CHECK(dp::encoded_size_of(sample.value) == sample.encoded_length);
    }
    SECTION("with decode")
    {
        simple_test_input_stream inputStream(sample.encoded_bytes());

        std::remove_cvref_t<decltype(sample.value)> value;
        REQUIRE(dp::decode(inputStream, value));

        CHECK(value == sample.value);
    }
}

TEST_CASE("encodes three tuples")
{
    constexpr item_sample_ct<std::tuple<int, long long, long>> sample{
            {   3, 22,  5},
            4, { 0x83,  3, 22,   5}
    };

    SECTION("to a stream")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        REQUIRE(dp::encode(outputStream, sample.value));

        CHECK_BLOB_EQ(outputStream.written(), sample.encoded_bytes());
    }
    SECTION("with as size_of operator")
    {
        CHECK(dp::encoded_size_of(sample.value) == sample.encoded_length);
    }
    SECTION("with decode")
    {
        simple_test_input_stream inputStream(sample.encoded_bytes());

        std::remove_cvref_t<decltype(sample.value)> value;
        REQUIRE(dp::decode(inputStream, value));

        CHECK(value == sample.value);
    }
}

} // namespace dp_tests

// NOLINTEND(readability-function-cognitive-complexity)
