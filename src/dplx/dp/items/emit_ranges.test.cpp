
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/emit_ranges.hpp"

#include <algorithm>
#include <forward_list>
#include <numeric>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <fmt/ranges.h>

#include <dplx/dp/api.hpp>
#include <dplx/dp/codecs/core.hpp>
#include <dplx/dp/indefinite_range.hpp>
#include <dplx/dp/map_pair.hpp>

#include "item_sample_ct.hpp"
#include "range_generator.hpp"
#include "simple_encodable.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"
#include "yaml_sample_generator.hpp"

namespace dp_tests
{

TEST_CASE("emit_array loops over a range of encodables and emits them")
{
    item_sample_rt<std::vector<int>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<int>>("arrays.yaml",
                                                                "int arrays"));
    INFO(sample);

    simple_test_emit_context ctx(sample.encoded.size());

    SECTION("with a sized range")
    {
        REQUIRE(emit_array(ctx.as_emit_context(), sample.value));

        REQUIRE(ctx.stream.written().size() == sample.encoded.size());
        CHECK(std::ranges::equal(ctx.stream.written(), sample.encoded_bytes()));
    }
    SECTION("with a forward range")
    {
        REQUIRE(emit_array(ctx.as_emit_context(),
                           dp::indefinite_range(sample.value)));

        REQUIRE(ctx.stream.written().size() == sample.encoded.size());
        CHECK(std::ranges::equal(ctx.stream.written(), sample.encoded_bytes()));
    }
}

TEST_CASE("emit_array can handle various data types")
{
    constexpr simple_encodable sample[] = {{0b1110'1001}, {0b1110'1001}};
    constexpr std::array expected{to_byte(dp::type_code::array) | std::byte{2U},
                                  std::byte{sample[0].value},
                                  std::byte{sample[1].value}};

    simple_test_emit_context ctx(3U);

    SECTION("standard C-Array")
    {
        REQUIRE(emit_array(ctx.as_emit_context(), sample));

        CHECK(std::ranges::equal(ctx.stream.written(), expected));
    }
    SECTION("std array")
    {
        constexpr std::array<simple_encodable, 2> input{sample[0], sample[1]};
        REQUIRE(emit_array(ctx.as_emit_context(), input));

        CHECK(std::ranges::equal(ctx.stream.written(), expected));
    }
}

TEST_CASE("emit_array_indefinite loops over an input range of encodables and "
          "emits them")
{
    item_sample_rt<std::vector<int>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<int>>(
                    "arrays.yaml", "indefinite int arrays"));
    INFO(sample);

    simple_test_emit_context ctx(sample.encoded.size());

    REQUIRE(emit_array_indefinite(ctx.as_emit_context(),
                                  dp::indefinite_range(sample.value)));

    REQUIRE(ctx.stream.written().size() == sample.encoded.size());
    CHECK(std::ranges::equal(ctx.stream.written(), sample.encoded_bytes()));
}

TEST_CASE("emit_map loops over a range of encodable pairs and emits them")
{
    item_sample_rt<std::vector<std::pair<int, int>>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<std::pair<int, int>>>(
                    "maps.yaml", "int maps"));
    INFO(sample);

    simple_test_emit_context ctx(sample.encoded.size());

    SECTION("with a sized range")
    {
        REQUIRE(emit_map(ctx.as_emit_context(), sample.value));

        REQUIRE(ctx.stream.written().size() == sample.encoded.size());
        CHECK(std::ranges::equal(ctx.stream.written(), sample.encoded_bytes()));
    }
    SECTION("with a forward range")
    {
        REQUIRE(emit_map(ctx.as_emit_context(),
                         dp::indefinite_range(sample.value)));

        REQUIRE(ctx.stream.written().size() == sample.encoded.size());
        CHECK(std::ranges::equal(ctx.stream.written(), sample.encoded_bytes()));
    }
}

TEST_CASE("emit_map_indefinite loops over an input range of encodable pairs "
          "and emits them")
{
    item_sample_rt<std::vector<std::pair<int, int>>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<std::pair<int, int>>>(
                    "maps.yaml", "indefinite int maps"));
    INFO(sample);

    simple_test_emit_context ctx(sample.encoded.size());

    REQUIRE(emit_map_indefinite(ctx.as_emit_context(),
                                dp::indefinite_range(sample.value)));

    REQUIRE(ctx.stream.written().size() == sample.encoded.size());
    CHECK(std::ranges::equal(ctx.stream.written(), sample.encoded_bytes()));
}

} // namespace dp_tests
