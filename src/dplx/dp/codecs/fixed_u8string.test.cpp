
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/fixed_u8string.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <dplx/dp/api.hpp>

#include "blob_matcher.hpp"
#include "item_sample_ct.hpp"
#include "test_input_stream.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

// NOLINTBEGIN(readability-function-cognitive-complexity)

namespace dp_tests
{

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
static_assert(std::same_as<dp::fixed_u8string<5>,
                           decltype(dp::fixed_u8string(u8"hello"))>);

static_assert(
        std::same_as<dp::fixed_u8string<16>,
                     typename std::common_type<dp::fixed_u8string<1>,
                                               dp::fixed_u8string<5>>::type>);
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

TEST_CASE("fixed_u8string has a codec")
{
    item_sample_ct<dp::fixed_u8string<4>> const sample{
            u8"some", 5, {0x64, u8's', u8'o', u8'm', u8'e'}
    };

    SECTION("with encode support")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        REQUIRE(dp::encode(outputStream, sample.value));

        CHECK_BLOB_EQ(outputStream.written(), sample.encoded_bytes());
    }
    SECTION("with a size_of operator")
    {
        CHECK(dp::encoded_size_of(sample.value) == sample.encoded_length);
    }
    SECTION("with decode support")
    {
        simple_test_input_stream inputStream(sample.encoded_bytes());

        std::remove_cvref_t<decltype(sample.value)> value;
        REQUIRE(dp::decode(inputStream, value));

        CHECK_BLOB_EQ(as_bytes(std::span(value)),
                      as_bytes(std::span(sample.value)));
    }
}

} // namespace dp_tests

// NOLINTEND(readability-function-cognitive-complexity)
