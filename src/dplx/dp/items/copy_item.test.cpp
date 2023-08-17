
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/copy_item.hpp"

#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <dplx/dp/streams/dynamic_memory_output_stream.hpp>

#include "blob_matcher.hpp"
#include "core_samples.hpp"
#include "item_sample_rt.hpp"
#include "range_generator.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"
#include "yaml_sample_generator.hpp"

namespace dp_tests
{

TEST_CASE("copy_item_to can copy posint items")
{
    auto const sample = GENERATE(borrowed_range(posint_samples));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());
    dp::dynamic_memory_output_stream<std::allocator<std::byte>> out;

    REQUIRE(dp::copy_item_to(ctx.as_parse_context(), out));

    CHECK(ctx.stream.input_size() == 0U);
    auto const written = std::move(out).written();
    CHECK_BLOB_EQ(written, sample.encoded_bytes());
}

TEST_CASE("copy_item_to can copy negint items")
{
    auto const sample = GENERATE(borrowed_range(negint_samples));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());
    dp::dynamic_memory_output_stream<std::allocator<std::byte>> out;

    REQUIRE(dp::copy_item_to(ctx.as_parse_context(), out));

    CHECK(ctx.stream.input_size() == 0U);
    auto const written = std::move(out).written();
    CHECK_BLOB_EQ(written, sample.encoded_bytes());
}

TEST_CASE("copy_item_to can copy blob items")
{
    item_sample_rt<YAML::Binary> const sample
            = GENERATE(load_samples_from_yaml<YAML::Binary>("blobs.yaml",
                                                            "definite blobs"));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded);
    dp::dynamic_memory_output_stream<std::allocator<std::byte>> out;

    REQUIRE(dp::copy_item_to(ctx.as_parse_context(), out));

    CHECK(ctx.stream.input_size() == 0U);
    auto const written = std::move(out).written();
    CHECK_BLOB_EQ(written, sample.encoded_bytes());
}

TEST_CASE("copy_item_to can copy indefinite blob items")
{
    item_sample_rt<YAML::Binary> const sample
            = GENERATE(load_samples_from_yaml<YAML::Binary>(
                    "blobs.yaml", "indefinite blobs"));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded);
    dp::dynamic_memory_output_stream<std::allocator<std::byte>> out;

    REQUIRE(dp::copy_item_to(ctx.as_parse_context(), out));

    CHECK(ctx.stream.input_size() == 0U);
    auto const written = std::move(out).written();
    CHECK_BLOB_EQ(written, sample.encoded_bytes());
}

TEST_CASE("copy_item_to can copy int array items")
{
    item_sample_rt<std::vector<int>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<int>>("arrays.yaml",
                                                                "int arrays"));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());
    dp::dynamic_memory_output_stream<std::allocator<std::byte>> out;

    REQUIRE(dp::copy_item_to(ctx.as_parse_context(), out));

    CHECK(ctx.stream.input_size() == 0U);
    auto const written = std::move(out).written();
    CHECK_BLOB_EQ(written, sample.encoded_bytes());
}

TEST_CASE("copy_item_to can copy indefinite int array items")
{
    item_sample_rt<std::vector<int>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<int>>(
                    "arrays.yaml", "indefinite int arrays"));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());
    dp::dynamic_memory_output_stream<std::allocator<std::byte>> out;

    REQUIRE(dp::copy_item_to(ctx.as_parse_context(), out));

    CHECK(ctx.stream.input_size() == 0U);
    auto const written = std::move(out).written();
    CHECK_BLOB_EQ(written, sample.encoded_bytes());
}

TEST_CASE("copy_item_to can copy int map items")
{
    item_sample_rt<std::vector<std::pair<int, int>>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<std::pair<int, int>>>(
                    "maps.yaml", "int maps"));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());
    dp::dynamic_memory_output_stream<std::allocator<std::byte>> out;

    REQUIRE(dp::copy_item_to(ctx.as_parse_context(), out));

    CHECK(ctx.stream.input_size() == 0U);
    auto const written = std::move(out).written();
    CHECK_BLOB_EQ(written, sample.encoded_bytes());
}

TEST_CASE("copy_item_to can copy indefinite int map items")
{
    item_sample_rt<std::vector<std::pair<int, int>>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<std::pair<int, int>>>(
                    "maps.yaml", "indefinite int maps"));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());
    dp::dynamic_memory_output_stream<std::allocator<std::byte>> out;

    REQUIRE(dp::copy_item_to(ctx.as_parse_context(), out));

    CHECK(ctx.stream.input_size() == 0U);
    auto const written = std::move(out).written();
    CHECK_BLOB_EQ(written, sample.encoded_bytes());
}

TEST_CASE("copy_item_to can copy float single items")
{
    auto const sample = GENERATE(borrowed_range(float_single_samples));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());
    dp::dynamic_memory_output_stream<std::allocator<std::byte>> out;

    REQUIRE(dp::copy_item_to(ctx.as_parse_context(), out));

    CHECK(ctx.stream.input_size() == 0U);
    auto const written = std::move(out).written();
    CHECK_BLOB_EQ(written, sample.encoded_bytes());
}

TEST_CASE("copy_item_to can copy float double items")
{
    auto const sample = GENERATE(borrowed_range(float_double_samples));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());
    dp::dynamic_memory_output_stream<std::allocator<std::byte>> out;

    REQUIRE(dp::copy_item_to(ctx.as_parse_context(), out));

    CHECK(ctx.stream.input_size() == 0U);
    auto const written = std::move(out).written();
    CHECK_BLOB_EQ(written, sample.encoded_bytes());
}

} // namespace dp_tests
