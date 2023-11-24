
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/parse_core.hpp"

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "core_samples.hpp"
#include "item_sample_ct.hpp"
#include "range_generator.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"

// NOLINTBEGIN(readability-function-cognitive-complexity)

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

    auto const parsed = dp::detail::do_parse_item_head<true, true>(
            ctx.as_parse_context());
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

        auto const parsed = dp::detail::do_parse_item_head<false, true>(
                ctx.as_parse_context());
        REQUIRE(parsed);
        auto const &head = parsed.assume_value();

        CHECK(head == sample.value);
        CHECK(ctx.stream.discarded() == sample.encoded_length);
    }
    SECTION("from an exactly sized buffer")
    {
        simple_test_parse_context ctx(sample.encoded_bytes());

        auto const parsed = dp::detail::do_parse_item_head<false, true>(
                ctx.as_parse_context());
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

// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
struct expect_acceptance_sample
{
    dp::type_code type;
    std::uint64_t value;
    std::array<std::uint8_t, dp::detail::var_uint_max_size> encoded;

    [[nodiscard]] auto encoded_bytes() const noexcept
            -> std::span<std::byte const>
    {
        return as_bytes(std::span(encoded));
    }
};

constexpr expect_acceptance_sample expect_acceptance_samples[] = {
        { dp::type_code::posint,    0U,       {0x00}},
        { dp::type_code::negint,   24U, {0x38, 0x18}},
        {dp::type_code::special, 0x1fU,       {0xff}},
        { dp::type_code::posint,   23U, {0x18, 0x17}}
};

TEST_CASE("expect_item_head accepts valid input")
{
    auto const sample = GENERATE(borrowed_range(expect_acceptance_samples));

    simple_test_parse_context ctx(sample.encoded_bytes());

    CHECK(dp::expect_item_head(ctx.as_parse_context(), sample.type,
                               sample.value));
}

// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
struct expect_rejection_sample
{
    dp::errc code;
    dp::type_code type;
    std::uint64_t value;
    std::array<std::uint8_t, dp::detail::var_uint_max_size> encoded;

    [[nodiscard]] auto encoded_bytes() const noexcept
            -> std::span<std::byte const>
    {
        return as_bytes(std::span(encoded));
    }
};

constexpr expect_rejection_sample expect_rejection_samples[] = {
        {
         dp::errc::item_type_mismatch,
         dp::type_code::special,
         0U,       {0x00},
         },
        {
         dp::errc::item_type_mismatch,
         dp::type_code::special,
         22U,       {0xff},
         },
        {
         dp::errc::invalid_additional_information,
         dp::type_code::negint,
         24U, {0x3f, 0x18},
         },
        {
         dp::errc::indefinite_item,
         dp::type_code::binary,
         21U,       {0x5f},
         },
        {
         dp::errc::item_value_out_of_range,
         dp::type_code::array,
         21U,       {0x82},
         },
};

TEST_CASE("expect_item_head rejects invalid input")
{
    auto const sample = GENERATE(borrowed_range(expect_rejection_samples));

    simple_test_parse_context ctx(sample.encoded_bytes());

    CHECK(dp::expect_item_head(ctx.as_parse_context(), sample.type,
                               sample.value)
                  .error()
          == sample.code);
}

TEMPLATE_TEST_CASE("parse_integer parses positive integers",
                   "",
                   unsigned char,
                   signed char,
                   unsigned short,
                   short,
                   unsigned,
                   int,
                   unsigned long,
                   long,
                   unsigned long long,
                   long long)
{
    item_sample_ct<unsigned long long> const sample
            = GENERATE(borrowed_range(posint_samples));
    INFO(sample);

    SECTION("with a fitting buffer")
    {
        simple_test_parse_context ctx(sample.encoded_bytes());

        TestType value;
        result<void> rx
                = dp::parse_integer<TestType>(ctx.as_parse_context(), value);
        if (std::in_range<TestType>(sample.value))
        {
            REQUIRE(rx);

            CHECK(ctx.stream.discarded() == sample.encoded_length);
            CHECK(value == sample.as<TestType>().value);
        }
        else
        {
            CHECK(rx.error() == dp::errc::item_value_out_of_range);
        }
    }
    SECTION("with an oversized buffer")
    {
        simple_test_parse_context ctx(as_bytes(std::span(sample.encoded)));

        TestType value;
        result<void> rx
                = dp::parse_integer<TestType>(ctx.as_parse_context(), value);
        if (std::in_range<TestType>(sample.value))
        {
            REQUIRE(rx);

            CHECK(ctx.stream.discarded() == sample.encoded_length);
            CHECK(value == sample.as<TestType>().value);
        }
        else
        {
            CHECK(rx.error() == dp::errc::item_value_out_of_range);
        }
    }
}

TEMPLATE_TEST_CASE("parse_integer parses negative integers correctly",
                   "",
                   signed char,
                   short,
                   int,
                   long,
                   long long)
{
    item_sample_ct<long long> const sample
            = GENERATE(borrowed_range(negint_samples));
    INFO(sample);

    SECTION("with a fitting buffer")
    {
        simple_test_parse_context ctx(sample.encoded_bytes());

        TestType value;
        result<void> rx
                = dp::parse_integer<TestType>(ctx.as_parse_context(), value);
        if (std::in_range<TestType>(sample.value))
        {
            REQUIRE(rx);

            CHECK(ctx.stream.discarded() == sample.encoded_length);
            CHECK(value == sample.as<TestType>().value);
        }
        else
        {
            CHECK(rx.error() == dp::errc::item_value_out_of_range);
        }
    }
    SECTION("with an oversized buffer")
    {
        simple_test_parse_context ctx(as_bytes(std::span(sample.encoded)));

        TestType value;
        result<void> rx
                = dp::parse_integer<TestType>(ctx.as_parse_context(), value);
        if (std::in_range<TestType>(sample.value))
        {
            REQUIRE(rx);

            CHECK(ctx.stream.discarded() == sample.encoded_length);
            CHECK(value == sample.as<TestType>().value);
        }
        else
        {
            CHECK(rx.error() == dp::errc::item_value_out_of_range);
        }
    }
}

constexpr dp::type_code unsigned_type_rejections[] = {
        dp::type_code::negint,     dp::type_code::binary,
        dp::type_code::text,       dp::type_code::array,
        dp::type_code::map,        dp::type_code::tag,
        dp::type_code::bool_false, dp::type_code::null,
        dp::type_code::undefined,
};

TEST_CASE("parse_integer<unsigned> rejects other major types")
{
    auto const majorType = GENERATE(borrowed_range(unsigned_type_rejections));
    std::byte memory[] = {to_byte(majorType)};

    simple_test_parse_context ctx(memory);

    unsigned value{};
    REQUIRE(dp::parse_integer<unsigned>(ctx.as_parse_context(), value).error()
            == dp::errc::item_type_mismatch);
}

constexpr dp::type_code signed_type_rejections[] = {
        dp::type_code::binary, dp::type_code::text,
        dp::type_code::array,  dp::type_code::map,
        dp::type_code::tag,    dp::type_code::bool_false,
        dp::type_code::null,   dp::type_code::undefined,
};

TEST_CASE("parse_integer<int> rejects other major types")
{
    auto const majorType = GENERATE(borrowed_range(signed_type_rejections));
    std::byte memory[] = {to_byte(majorType)};

    simple_test_parse_context ctx(memory);

    int value{};
    REQUIRE(dp::parse_integer<int>(ctx.as_parse_context(), value).error()
            == dp::errc::item_type_mismatch);
}

TEST_CASE("boolean parses correctly")
{
    auto const sample = GENERATE(gens::values<item_sample_ct<bool>>({
            {false, 1, {0b111'10100}},
            { true, 1, {0b111'10101}},
    }));

    simple_test_parse_context ctx(sample.encoded_bytes());
    bool value{};
    REQUIRE(dp::parse_boolean(ctx.as_parse_context(), value));

    CHECK(value == sample.value);
    CHECK(ctx.stream.discarded() == sample.encoded_length);
}

TEST_CASE("parse_floating_point parses large double")
{
    auto const sample = GENERATE(borrowed_range(float_double_samples));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());

    double value{};
    REQUIRE(dp::parse_floating_point(ctx.as_parse_context(), value));

    if (std::isnan(sample.value))
    {
        CHECK(std::isnan(value));
    }
    else
    {
        CHECK(value == sample.value);
    }
    CHECK(ctx.stream.discarded() == sample.encoded_length);
}
TEST_CASE("parse_floating_point parses small double")
{
    auto const sample = GENERATE(borrowed_range(float_single_samples));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());

    double value{};
    REQUIRE(dp::parse_floating_point(ctx.as_parse_context(), value));

    if (std::isnan(sample.value))
    {
        CHECK(std::isnan(value));
    }
    else
    {
        CHECK(value == sample.as<double>().value);
    }
    CHECK(ctx.stream.discarded() == sample.encoded_length);
}

TEST_CASE("parse_floating_point parses float")
{
    auto const sample = GENERATE(borrowed_range(float_single_samples));
    INFO(sample);

    simple_test_parse_context ctx(sample.encoded_bytes());

    float value{};
    REQUIRE(dp::parse_floating_point(ctx.as_parse_context(), value));

    if (std::isnan(sample.value))
    {
        CHECK(std::isnan(value));
    }
    else
    {
        CHECK(value == sample.value);
    }
    CHECK(ctx.stream.discarded() == sample.encoded_length);
}

} // namespace dp_tests

// NOLINTEND(readability-function-cognitive-complexity)
