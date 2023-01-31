
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/parse_ranges.hpp"

#include <numeric>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <dplx/dp/cpos/container.std.hpp>

#include "blob_matcher.hpp"
#include "dplx/dp/api.hpp"
#include "dplx/dp/codecs/core.hpp"
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

TEST_CASE("parse_array parses a finite array of integers into a vector")
{
    item_sample_rt<std::vector<int>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<int>>("arrays.yaml",
                                                                "int arrays"));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());
    std::vector<int> value;
    auto decodeElement = [](dp::parse_context &lctx, std::vector<int> &dest,
                            std::size_t const) noexcept -> dp::result<void>
    {
        return dp::decode(lctx, dest.emplace_back());
    };

    SECTION("with flexible")
    {
        auto parseRx
                = dp::parse_array(ctx.as_parse_context(), value, decodeElement);

        REQUIRE(parseRx);
        CHECK(parseRx.assume_value() == sample.value.size());

        CHECK(std::ranges::equal(value, sample.value));
    }
    SECTION("with finite")
    {
        auto parseRx = dp::parse_array_finite(ctx.as_parse_context(), value,
                                              decodeElement);

        REQUIRE(parseRx);
        CHECK(parseRx.assume_value() == sample.value.size());

        CHECK(std::ranges::equal(value, sample.value));
    }
}

TEST_CASE("parse_array parses an indefinite array of integers into a vector")
{
    item_sample_rt<std::vector<int>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<int>>(
                    "arrays.yaml", "indefinite int arrays"));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());
    std::vector<int> value;
    auto decodeElement = [](dp::parse_context &lctx, std::vector<int> &dest,
                            std::size_t const) noexcept -> dp::result<void>
    {
        return dp::decode(lctx, dest.emplace_back());
    };

    SECTION("with flexible")
    {
        auto parseRx
                = dp::parse_array(ctx.as_parse_context(), value, decodeElement);

        REQUIRE(parseRx);
        CHECK(parseRx.assume_value() == sample.value.size());

        CHECK(std::ranges::equal(value, sample.value));
    }
    SECTION("with finite rejection")
    {
        CHECK(dp::parse_array_finite(ctx.as_parse_context(), value,
                                     decodeElement)
                      .error()
              == dp::errc::indefinite_item);
    }
}

TEST_CASE("parse_map parses a finite map of integers into a vector of pairs")
{
    item_sample_rt<std::vector<std::pair<int, int>>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<std::pair<int, int>>>(
                    "maps.yaml", "int maps"));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());
    std::vector<std::pair<int, int>> value;
    auto decodePair = [](dp::parse_context &lctx,
                         std::vector<std::pair<int, int>> &dest,
                         std::size_t const) noexcept -> dp::result<void>
    {
        auto &pair = dest.emplace_back();
        DPLX_TRY(dp::decode(lctx, pair.first));
        DPLX_TRY(dp::decode(lctx, pair.second));
        return dp::success();
    };

    SECTION("with flexible")
    {
        auto parseRx = dp::parse_map(ctx.as_parse_context(), value, decodePair);

        REQUIRE(parseRx);
        CHECK(parseRx.assume_value() == sample.value.size());

        CHECK(std::ranges::equal(value, sample.value));
    }
    SECTION("with finite")
    {
        auto parseRx = dp::parse_map_finite(ctx.as_parse_context(), value,
                                            decodePair);

        REQUIRE(parseRx);
        CHECK(parseRx.assume_value() == sample.value.size());

        CHECK(std::ranges::equal(value, sample.value));
    }
}

TEST_CASE(
        "parse_map parses an indefinite map of integers into a vector of pairs")
{
    item_sample_rt<std::vector<std::pair<int, int>>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<std::pair<int, int>>>(
                    "maps.yaml", "indefinite int maps"));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());
    std::vector<std::pair<int, int>> value;
    auto decodePair = [](dp::parse_context &lctx,
                         std::vector<std::pair<int, int>> &dest,
                         std::size_t const) noexcept -> dp::result<void>
    {
        auto &pair = dest.emplace_back();
        DPLX_TRY(dp::decode(lctx, pair.first));
        DPLX_TRY(dp::decode(lctx, pair.second));
        return dp::success();
    };

    SECTION("with flexible")
    {
        auto parseRx = dp::parse_map(ctx.as_parse_context(), value, decodePair);

        REQUIRE(parseRx);
        CHECK(parseRx.assume_value() == sample.value.size());

        CHECK(std::ranges::equal(value, sample.value));
    }
    SECTION("with finite rejection")
    {
        CHECK(dp::parse_map_finite(ctx.as_parse_context(), value, decodePair)
                      .error()
              == dp::errc::indefinite_item);
    }
}

} // namespace dp_tests

// NOLINTEND(readability-function-cognitive-complexity)
