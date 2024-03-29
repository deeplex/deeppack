
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/auto_enum.hpp"

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include "blob_matcher.hpp"
#include "item_sample_ct.hpp"
#include "range_generator.hpp"
#include "test_input_stream.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

// NOLINTBEGIN(readability-function-cognitive-complexity)

namespace
{

template <typename T>
struct enum_samples;

enum class signed_test_enum : int
{
    neg2 = -2,
    neg1 = -1,
    zero = 0,
    pos1 = 1,
    pos2 = 2,
};
static_assert(dplx::dp::encodable<signed_test_enum>);

template <>
struct enum_samples<signed_test_enum>
{
    static constexpr dp_tests::item_sample_ct<signed_test_enum> values[] = {
            {signed_test_enum::neg2, 1, {0x21}},
            {signed_test_enum::neg1, 1, {0x20}},
            {signed_test_enum::zero, 1, {0x00}},
            {signed_test_enum::pos1, 1, {0x01}},
            {signed_test_enum::pos2, 1, {0x02}},
    };
};

enum class unsigned_test_enum : unsigned
{
    zero = 0,
    pos1 = 1,
    pos2 = 2,
};
static_assert(dplx::dp::encodable<unsigned_test_enum>);

template <>
struct enum_samples<unsigned_test_enum>
{
    static constexpr dp_tests::item_sample_ct<unsigned_test_enum> values[] = {
            {unsigned_test_enum::zero, 1, {0x00}},
            {unsigned_test_enum::pos1, 1, {0x01}},
            {unsigned_test_enum::pos2, 1, {0x02}},
    };
};

enum unscoped_test_enum
{
    UTE_zero = 0,
    UTE_bound = 256,
};
static_assert(dplx::dp::encodable<unscoped_test_enum>);

template <>
struct enum_samples<unscoped_test_enum>
{
    static constexpr dp_tests::item_sample_ct<unscoped_test_enum> values[] = {
            { UTE_zero, 1,             {0x00}},
            {UTE_bound, 3, {0x19, 0x01, 0x00}},
    };
};

} // namespace

namespace dp_tests
{

static_assert(!dp::codable_enum<std::byte>);
static_assert(!dp::encodable<std::byte>);

TEMPLATE_TEST_CASE("enums have an auto codec",
                   "",
                   signed_test_enum,
                   unsigned_test_enum,
                   unscoped_test_enum)
{
    static_assert(dp::encodable<TestType>);
    static_assert(dp::decodable<TestType>);
    auto const sample
            = GENERATE(borrowed_range(enum_samples<TestType>::values));

    SECTION("with encode")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        REQUIRE(dp::encode(outputStream, sample.value));

        CHECK_BLOB_EQ(outputStream.written(), sample.encoded_bytes());
    }
    SECTION("with size_of")
    {
        CHECK(dp::encoded_size_of(sample.value) == sample.encoded_length);
    }
    SECTION("with decode")
    {
        simple_test_input_stream inputStream(sample.encoded_bytes());

        TestType value;
        REQUIRE(dp::decode(inputStream, value));

        CHECK(value == sample.value);
    }
}

} // namespace dp_tests

// NOLINTEND(readability-function-cognitive-complexity)
