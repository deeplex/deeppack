
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/std-string.hpp"

#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <dplx/dp/api.hpp>

#include "blob_matcher.hpp"
#include "item_sample_ct.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

// These APIs just dispatch to `dp::emit_u8string(…)`.
// Therefore we only test API conformance.
// Actual string emitter tests can be found in emit_core.test.cpp

TEST_CASE("std::u8string_view should be encodable")
{
    item_sample_ct<std::u8string_view> const sample{
            u8"some", 5, {0x64, u8's', u8'o', u8'm', u8'e'}
    };

    SECTION("to a stream")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        REQUIRE(dp::encode(outputStream, sample.value));

        CHECK_BLOB_EQ(outputStream.written(), sample.encoded_bytes());
    }
    SECTION("with a size_of operator")
    {
        CHECK(dp::encoded_size_of(sample.value) == sample.encoded_length);
    }
}

TEST_CASE("std::u8string should be encodable")
{
    item_sample_ct<std::u8string> const sample{
            u8"some", 5, {0x64, u8's', u8'o', u8'm', u8'e'}
    };

    SECTION("to a stream")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        REQUIRE(dp::encode(outputStream, sample.value));

        CHECK_BLOB_EQ(outputStream.written(), sample.encoded_bytes());
    }
    SECTION("with a size_of operator")
    {
        CHECK(dp::encoded_size_of(sample.value) == sample.encoded_length);
    }
}

TEST_CASE("std::string_view should be encodable")
{
    item_sample_ct<std::string_view> const sample{
            "some", 5, {0x64, u8's', u8'o', u8'm', u8'e'}
    };

    SECTION("to a stream")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        REQUIRE(dp::encode(outputStream, sample.value));

        CHECK_BLOB_EQ(outputStream.written(), sample.encoded_bytes());
    }
    SECTION("with a size_of operator")
    {
        CHECK(dp::encoded_size_of(sample.value) == sample.encoded_length);
    }
}

TEST_CASE("std::string should be encodable")
{
    item_sample_ct<std::string> const sample{
            "some", 5, {0x64, u8's', u8'o', u8'm', u8'e'}
    };

    SECTION("to a stream")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        REQUIRE(dp::encode(outputStream, sample.value));

        CHECK_BLOB_EQ(outputStream.written(), sample.encoded_bytes());
    }
    SECTION("with a size_of operator")
    {
        CHECK(dp::encoded_size_of(sample.value) == sample.encoded_length);
    }
}

} // namespace dp_tests
