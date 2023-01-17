
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/parse_ranges.hpp"

#include <numeric>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <dplx/dp/customization.std.hpp>

#include "blob_matcher.hpp"
#include "item_sample_rt.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"
#include "yaml_sample_generator.hpp"

// NOLINTBEGIN(readability-function-cognitive-complexity)

namespace dp_tests
{

TEST_CASE("parse a short finite blob")
{
    item_sample_rt<YAML::Binary> const sample
            = GENERATE(load_samples_from_yaml<YAML::Binary>("blobs.yaml",
                                                            "definite blobs"));
    INFO(sample);
    std::span const sampleValue(sample.value.size() == 0U ? nullptr
                                                          : sample.value.data(),
                                sample.value.size());

    simple_test_parse_context ctx(sample.encoded);

    std::vector<std::byte> value;

    SECTION("with flexible")
    {
        auto parseRx = dp::parse_binary(ctx.as_parse_context(), value);
        REQUIRE(parseRx);
        CHECK(parseRx.assume_value() == sample.value.size());

        CHECK_BLOB_EQ(value, sampleValue);
    }
    SECTION("with _finite")
    {
        auto parseRx = dp::parse_binary_finite(ctx.as_parse_context(), value);
        REQUIRE(parseRx);
        CHECK(parseRx.assume_value() == sample.value.size());

        CHECK_BLOB_EQ(value, sampleValue);
    }
}

TEST_CASE("parse a short indefinite blob")
{
    item_sample_rt<YAML::Binary> const sample
            = GENERATE(load_samples_from_yaml<YAML::Binary>(
                    "blobs.yaml", "indefinite blobs"));
    INFO(sample);
    std::span const sampleValue(sample.value.size() == 0U ? nullptr
                                                          : sample.value.data(),
                                sample.value.size());

    simple_test_parse_context ctx(sample.encoded);

    std::vector<std::byte> value;

    SECTION("with flexible")
    {
        auto parseRx = dp::parse_binary(ctx.as_parse_context(), value);
        REQUIRE(parseRx);
        CHECK(parseRx.assume_value() == sample.value.size());

        CHECK_BLOB_EQ(value, sampleValue);
    }
    SECTION("with _finite")
    {
        REQUIRE(dp::parse_binary_finite(ctx.as_parse_context(), value).error()
                == dp::errc::indefinite_item);
    }
}

TEST_CASE("parse a short finite text")
{
    item_sample_rt<std::string> const sample = GENERATE(
            load_samples_from_yaml<std::string>("text.yaml", "definite texts"));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded);

    std::string value;

    SECTION("with flexible")
    {
        auto parseRx = dp::parse_text(ctx.as_parse_context(), value);
        REQUIRE(parseRx);
        CHECK(parseRx.assume_value() == sample.value.size());

        CHECK_BLOB_EQ(value, sample.value);
    }
    SECTION("with _finite")
    {
        auto parseRx = dp::parse_text_finite(ctx.as_parse_context(), value);
        REQUIRE(parseRx);
        CHECK(parseRx.assume_value() == sample.value.size());

        CHECK_BLOB_EQ(value, sample.value);
    }
}

TEST_CASE("parse a short indefinite text")
{
    item_sample_rt<std::string> const sample
            = GENERATE(load_samples_from_yaml<std::string>("text.yaml",
                                                           "indefinite texts"));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded);

    std::string value;

    SECTION("with flexible")
    {
        auto parseRx = dp::parse_text(ctx.as_parse_context(), value);
        REQUIRE(parseRx);
        CHECK(parseRx.assume_value() == sample.value.size());

        CHECK_BLOB_EQ(value, sample.value);
    }
    SECTION("with _finite")
    {
        REQUIRE(dp::parse_text_finite(ctx.as_parse_context(), value).error()
                == dp::errc::indefinite_item);
    }
}

} // namespace dp_tests

// NOLINTEND(readability-function-cognitive-complexity)
