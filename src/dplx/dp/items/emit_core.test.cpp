
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/emit_core.hpp"

#include <array>
#include <numeric>
#include <string>
#include <string_view>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <dplx/cncr/misc.hpp>

#include "blob_matcher.hpp"
#include "core_samples.hpp"
#include "item_sample_ct.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

TEST_CASE("boolean emits correctly")
{
    auto sample = GENERATE(gens::values<item_sample_ct<bool>>({
            {false, 1, {0b111'10100}},
            { true, 1, {0b111'10101}},
    }));

    simple_test_output_stream outputStream(sample.encoded_length);

    dp::emit_context ctx{outputStream};
    REQUIRE(emit_boolean(ctx, sample.value));

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

TEMPLATE_TEST_CASE("positive integers emit correctly",
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
    auto sample = GENERATE(integer_samples<TestType>(posint_samples));
    INFO(sample);

    SECTION("with a fitting buffer")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        dp::emit_context ctx{outputStream};
        REQUIRE(dp::emit_integer(ctx, sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }

    SECTION("with an oversized buffer")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        dp::emit_context ctx{outputStream};
        REQUIRE(dp::emit_integer(ctx, sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }
}

TEMPLATE_TEST_CASE("negative integers emit correctly",
                   "",
                   signed char,
                   short,
                   int,
                   long,
                   long long)
{
    auto sample = GENERATE(integer_samples<TestType>(negint_samples));
    INFO(sample);

    SECTION("with a fitting buffer")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        dp::emit_context ctx{outputStream};
        REQUIRE(dp::emit_integer(ctx, sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }

    SECTION("with an oversized buffer")
    {
        simple_test_output_stream outputStream(dp::detail::var_uint_max_size);

        dp::emit_context ctx{outputStream};
        REQUIRE(dp::emit_integer(ctx, sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }
}

TEST_CASE("finite prefixes are emitted correctly")
{
    auto sample = GENERATE(integer_samples<std::size_t>(posint_samples));
    INFO(sample);

    simple_test_output_stream outputStream(sample.encoded_length);
    dp::emit_context ctx{outputStream};

    SECTION("for binary")
    {
        sample.encoded[0] |= static_cast<std::uint8_t>(dp::type_code::binary);
        REQUIRE(emit_binary(ctx, sample.value));
    }
    SECTION("for text")
    {
        sample.encoded[0] |= static_cast<std::uint8_t>(dp::type_code::text);
        REQUIRE(emit_u8string(ctx, sample.value));
    }
    SECTION("for array")
    {
        sample.encoded[0] |= static_cast<std::uint8_t>(dp::type_code::array);
        REQUIRE(emit_array(ctx, sample.value));
    }
    SECTION("for map")
    {
        sample.encoded[0] |= static_cast<std::uint8_t>(dp::type_code::map);
        REQUIRE(emit_map(ctx, sample.value));
    }
    SECTION("for tags")
    {
        sample.encoded[0] |= static_cast<std::uint8_t>(dp::type_code::tag);
        REQUIRE(emit_tag(ctx, sample.value));
    }

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

TEST_CASE("emit_binary writes a short blob")
{
    item_sample_ct<unsigned> const sample
            = GENERATE(borrowed_range(binary_samples));

    std::vector<unsigned char> generated(sample.value);
    std::iota(generated.begin(), generated.end(),
              static_cast<unsigned char>(1));

    simple_test_output_stream outputStream(sample.encoded_length);
    dp::emit_context ctx{outputStream};

    REQUIRE(emit_binary(
            ctx,
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<std::byte const *>(generated.data()),
            generated.size()));

    REQUIRE(outputStream.written().size() == sample.encoded_length);
    auto const prefix_length
            = std::min(outputStream.written().size(), sample.encoded.size());
    CHECK_BLOB_EQ(outputStream.written().first(prefix_length),
                  sample.encoded_bytes());
}

TEST_CASE("emit_u8string writes a short string")
{
    auto const sample = GENERATE(borrowed_range(u8string_samples));

    simple_test_output_stream outputStream(sample.encoded_length);
    dp::emit_context ctx{outputStream};

    SECTION("for char8_t const *")
    {
        REQUIRE(emit_u8string(ctx, sample.value.data(), sample.value.size()));
    }
    SECTION("for char const *")
    {
        auto const *const narrow
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                = reinterpret_cast<char const *>(sample.value.data());
        REQUIRE(emit_u8string(ctx, narrow, sample.value.size()));
    }

    REQUIRE(outputStream.written().size() == sample.encoded_length);
    auto const prefix_length
            = std::min(outputStream.written().size(), sample.encoded.size());
    CHECK_BLOB_EQ(outputStream.written().first(prefix_length),
                  sample.encoded_bytes());
}

TEST_CASE("indefinite prefixes are emitted correctly")
{
    simple_test_output_stream outputStream(1U);
    dp::emit_context ctx{outputStream};

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    std::byte expected{0x1f};
    SECTION("for binary")
    {
        expected |= static_cast<std::byte>(dp::type_code::binary);
        REQUIRE(emit_binary_indefinite(ctx));
    }
    SECTION("for text")
    {
        expected |= static_cast<std::byte>(dp::type_code::text);
        REQUIRE(emit_u8string_indefinite(ctx));
    }
    SECTION("for array")
    {
        expected |= static_cast<std::byte>(dp::type_code::array);
        REQUIRE(emit_array_indefinite(ctx));
    }
    SECTION("for map")
    {
        expected |= static_cast<std::byte>(dp::type_code::map);
        REQUIRE(emit_map_indefinite(ctx));
    }

    REQUIRE(outputStream.written().size() == 1U);
    CHECK(*outputStream.written().data() == expected);
}

TEST_CASE("float single emits correctly")
{
    auto sample = GENERATE(borrowed_range(float_single_samples));
    INFO(sample);

    simple_test_output_stream outputStream(sample.encoded_length);

    dp::emit_context ctx{outputStream};
    REQUIRE(emit_float_single(ctx, sample.value));

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

TEST_CASE("float double emits correctly")
{
    auto sample = GENERATE(borrowed_range(float_double_samples));
    INFO(sample);

    simple_test_output_stream outputStream(sample.encoded_length);

    dp::emit_context ctx{outputStream};
    REQUIRE(emit_float_double(ctx, sample.value));

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

TEST_CASE("null emits correctly")
{
    simple_test_output_stream outputStream(1U);

    dp::emit_context ctx{outputStream};
    REQUIRE(emit_null(ctx));

    REQUIRE(outputStream.written().size() == 1U);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    REQUIRE(*outputStream.written().data() == std::byte{0b111'10110});
}

TEST_CASE("undefined emits correctly")
{
    simple_test_output_stream outputStream(1U);

    dp::emit_context ctx{outputStream};
    REQUIRE(emit_undefined(ctx));

    REQUIRE(outputStream.written().size() == 1U);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    REQUIRE(*outputStream.written().data() == std::byte{0b111'10111});
}

TEST_CASE("break emits correctly")
{
    simple_test_output_stream outputStream(1U);

    dp::emit_context ctx{outputStream};
    REQUIRE(emit_break(ctx));

    REQUIRE(outputStream.written().size() == 1U);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    REQUIRE(*outputStream.written().data() == std::byte{0b111'11111});
}

} // namespace dp_tests
