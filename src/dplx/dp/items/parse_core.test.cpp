
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/parse_core.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "item_sample_ct.hpp"
#include "range_generator.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

using enum dp::type_code;

constexpr item_sample_ct<dp::item_head> parse_samples[] = {
  // clang-format off
              
    // Appendix A.Examples
    {
        {posint, {}, 1, 0},
        1, {0x00},
    },
    {
        {posint, {}, 1, 1},
        1, {0x01},
    },
    {
        {posint, {}, 1, 0x0a},
        1, {0x0a},
    },
    {
        {posint, {}, 2, 0x19},
        2, {0x18, 0x19},
    },
    {
        {posint, {}, 2, 0x64},
        2, {0x18, 0x64}
    },
    {
        {posint, {}, 3, 0x03e8},
        3, {0x19, 0x03, 0xe8},
    },
    {
        {posint, {}, 5, 0x000f'4240},
        5, {0x1a, 0x00, 0x0f, 0x42, 0x40},
    },
    {
        {posint, {}, 9, 0x0000'00e8'd4a5'1000},
        9, {0x1b, 0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00},
    },
    {
        {negint, {}, 1, 9},
        1, {0x29},
    },
    {
        {negint, {}, 2, 99},
        2, {0x38, 0x63},
    },
    {
        {negint, {}, 3, 999},
        3, {0x39, 0x03, 0xe7},
    },

    // posint boundaries
    {
        {posint, {}, 1, 0x17},
        1, {0x17}, 
    },
    {
        {posint, {}, 2, 0x18},
        2, {0x18, 0x18}
    },
    {
        {posint, {}, 2, 0xff},
        2, {0x18, 0xff},
    },
    {
        {posint, {}, 3, 0x0100},
        3, {0x19, 0x01, 0x00},
    },
    {
        {posint, {}, 3, 0xffff},
        3, {0x19, 0xff, 0xff},
    },
    {
        {posint, {}, 5, 0x1'0000},
        5, {0x1a, 0x00, 0x01, 0x00, 0x00},
    },
    {
        {posint, {}, 5, 0xffff'ffff},
        5, {0x1a, 0xff, 0xff, 0xff, 0xff},
    },
    {
        {posint, {}, 9, 0x1'0000'0000},
        9, {0x1b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00},
    },
    {
        {posint, {}, 9, 0xffff'ffff'ffff'ffff},
        9, {0x1b, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    },

    // special break
    {
        {special, dp::item_head::flag::indefinite, 1, 0x1f},
        1, {0xff},
    },

  // clang-format on
};

TEST_CASE("parse_item_head_speculative can parse basic item_heads")
{
    auto const sample = GENERATE(borrowed_range(parse_samples));
    INFO(sample);

    simple_test_parse_context ctx(as_bytes(std::span(sample.encoded)));

    auto const parsed
            = dp::detail::parse_item_head_speculative(ctx.as_parse_context());
    REQUIRE(parsed);
    auto const &head = parsed.assume_value();

    CHECK(head == sample.value);
    CHECK(ctx.stream.discarded() == sample.encoded_length);
}

TEST_CASE("parse_item_head_safe can parse basic item_heads")
{
    auto const sample = GENERATE(borrowed_range(parse_samples));
    INFO(sample);

    SECTION("from an oversized input")
    {
        simple_test_parse_context ctx(as_bytes(std::span(sample.encoded)));

        auto const parsed
                = dp::detail::parse_item_head_safe(ctx.as_parse_context());
        REQUIRE(parsed);
        auto const &head = parsed.assume_value();

        CHECK(head == sample.value);
        CHECK(ctx.stream.discarded() == sample.encoded_length);
    }
    SECTION("from an exactly sized buffer")
    {
        simple_test_parse_context ctx(sample.encoded_bytes());

        auto const parsed
                = dp::detail::parse_item_head_safe(ctx.as_parse_context());
        REQUIRE(parsed);
        auto const &head = parsed.assume_value();

        CHECK(head == sample.value);
        CHECK(ctx.stream.discarded() == sample.encoded_length);
    }
}

TEST_CASE("parse_item_head can parse basic item_heads")
{
    auto const sample = GENERATE(borrowed_range(parse_samples));
    INFO(sample);

    SECTION("from an oversized input")
    {
        simple_test_parse_context ctx(as_bytes(std::span(sample.encoded)));

        auto const parsed = dp::parse_item_head(ctx.as_parse_context());
        REQUIRE(parsed);
        auto const &head = parsed.assume_value();

        CHECK(head == sample.value);
        CHECK(ctx.stream.discarded() == sample.encoded_length);
    }
    SECTION("from an exactly sized buffer")
    {
        simple_test_parse_context ctx(sample.encoded_bytes());

        auto const parsed = dp::parse_item_head(ctx.as_parse_context());
        REQUIRE(parsed);
        auto const &head = parsed.assume_value();

        CHECK(head == sample.value);
        CHECK(ctx.stream.discarded() == sample.encoded_length);
    }
    SECTION("from a lazy buffer")
    {
        simple_test_parse_context ctx(sample.encoded_bytes(), true);

        auto const parsed = dp::parse_item_head(ctx.as_parse_context());
        REQUIRE(parsed);
        auto const &head = parsed.assume_value();

        CHECK(head == sample.value);
        CHECK(ctx.stream.discarded() == sample.encoded_length);
    }
}

} // namespace dp_tests
