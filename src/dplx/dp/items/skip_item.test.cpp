
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/skip_item.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "core_samples.hpp"
#include "item_sample_rt.hpp"
#include "range_generator.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"
#include "yaml_sample_generator.hpp"

namespace dp_tests
{

TEST_CASE("skip_item can skip posint items")
{
    auto const sample = GENERATE(borrowed_range(posint_samples));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());

    REQUIRE(dp::skip_item(ctx.as_parse_context()));

    CHECK(ctx.stream.input_size() == 0U);
}

TEST_CASE("skip_item can skip negint items")
{
    auto const sample = GENERATE(borrowed_range(negint_samples));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());

    REQUIRE(dp::skip_item(ctx.as_parse_context()));

    CHECK(ctx.stream.input_size() == 0U);
}

TEST_CASE("skip_item can skip int map items")
{
    item_sample_rt<std::vector<std::pair<int, int>>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<std::pair<int, int>>>(
                    "maps.yaml", "int maps"));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());

    REQUIRE(dp::skip_item(ctx.as_parse_context()));

    CHECK(ctx.stream.input_size() == 0U);
}

TEST_CASE("skip_item can skip indefinite int map items")
{
    item_sample_rt<std::vector<std::pair<int, int>>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<std::pair<int, int>>>(
                    "maps.yaml", "indefinite int maps"));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());

    REQUIRE(dp::skip_item(ctx.as_parse_context()));

    CHECK(ctx.stream.input_size() == 0U);
}

TEST_CASE("skip_item can skip float single items")
{
    auto const sample = GENERATE(borrowed_range(float_single_samples));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());

    REQUIRE(dp::skip_item(ctx.as_parse_context()));

    CHECK(ctx.stream.input_size() == 0U);
}

TEST_CASE("skip_item can skip float double items")
{
    auto const sample = GENERATE(borrowed_range(float_double_samples));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());

    REQUIRE(dp::skip_item(ctx.as_parse_context()));

    CHECK(ctx.stream.input_size() == 0U);
}

} // namespace dp_tests
