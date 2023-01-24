
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/std-container.hpp"

#include <array>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <vector>

#include <boost/container/deque.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/list.hpp>
#include <boost/container/map.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/container/vector.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <dplx/cncr/misc.hpp>

#include "blob_matcher.hpp"
#include "dplx/dp/api.hpp"
#include "dplx/dp/codecs/core.hpp"
#include "dplx/dp/indefinite_range.hpp"
#include "dplx/dp/streams/memory_output_stream2.hpp"
#include "item_sample_ct.hpp"
#include "item_sample_rt.hpp"
#include "test_input_stream.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"
#include "yaml_sample_generator.hpp"

// NOLINTBEGIN(readability-function-cognitive-complexity)

namespace dp_tests
{

////////////////////////////////////////////////////////////////////////////////
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)

static_assert(dp::sequence_container<std::deque<int>>);
static_assert(dp::sequence_container<std::list<int>>);
static_assert(dp::sequence_container<std::vector<int>>);
static_assert(dp::sequence_container<boost::container::deque<int>>);
static_assert(dp::sequence_container<boost::container::list<int>>);
static_assert(dp::sequence_container<boost::container::small_vector<int, 33>>);
static_assert(dp::sequence_container<boost::container::static_vector<int, 37>>);
static_assert(dp::sequence_container<boost::container::vector<int>>);

////////////////////////////////////////////////////////////////////////////////
// encodable
static_assert(dp::ng::encodable<std::deque<int>>);
static_assert(dp::ng::encodable<std::list<int>>);
static_assert(dp::ng::encodable<std::vector<int>>);
static_assert(dp::ng::encodable<boost::container::deque<int>>);
static_assert(dp::ng::encodable<boost::container::list<int>>);
static_assert(dp::ng::encodable<boost::container::small_vector<int, 33>>);
static_assert(dp::ng::encodable<boost::container::static_vector<int, 37>>);
static_assert(dp::ng::encodable<boost::container::vector<int>>);

static_assert(dp::ng::encodable<std::set<int>>);
static_assert(dp::ng::encodable<std::map<int, int>>);
static_assert(dp::ng::encodable<std::unordered_map<int, int>>);
static_assert(dp::ng::encodable<boost::container::map<int, int>>);
static_assert(dp::ng::encodable<boost::container::flat_map<int, int>>);

static_assert(dp::ng::encodable<std::array<std::byte, 15>>);
static_assert(dp::ng::encodable<std::array<std::byte, 16>>);
static_assert(dp::ng::encodable<std::array<int, 15>>);
static_assert(dp::ng::encodable<std::array<unsigned, 16>>);

////////////////////////////////////////////////////////////////////////////////
// decodable
static_assert(dp::ng::decodable<std::deque<int>>);
static_assert(dp::ng::decodable<std::list<int>>);
static_assert(dp::ng::decodable<std::vector<int>>);
static_assert(dp::ng::decodable<boost::container::deque<int>>);
static_assert(dp::ng::decodable<boost::container::list<int>>);
static_assert(dp::ng::decodable<boost::container::small_vector<int, 33>>);
static_assert(dp::ng::decodable<boost::container::static_vector<int, 37>>);
static_assert(dp::ng::decodable<boost::container::vector<int>>);

static_assert(dp::ng::decodable<std::set<int>>);
static_assert(dp::ng::decodable<std::map<int, int>>);
static_assert(dp::ng::decodable<std::unordered_map<int, int>>);
static_assert(dp::ng::decodable<boost::container::map<int, int>>);
static_assert(dp::ng::decodable<boost::container::flat_map<int, int>>);

static_assert(dp::ng::decodable<std::array<std::byte, 15>>);
static_assert(dp::ng::decodable<std::array<std::byte, 16>>);
static_assert(dp::ng::decodable<std::array<int, 15>>);
static_assert(dp::ng::decodable<std::array<unsigned, 16>>);

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
////////////////////////////////////////////////////////////////////////////////

TEST_CASE("std::vector of bytes has a codec")
{
    item_sample_ct<std::array<std::byte, 2U>> const sample{
            .value = cncr::make_byte_array<2U>({0xfe, 0xfe}
              ),
            .encoded_length = 3,
            .encoded = { 0x42, 0xfe,0xfe},
    };
    std::vector<std::byte> sampleValue(sample.value.begin(),
                                       sample.value.end());

    SECTION("with encode")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        REQUIRE(dp::encode(outputStream, sampleValue));

        CHECK_BLOB_EQ(outputStream.written(), sample.encoded_bytes());
    }
    SECTION("with size_of")
    {
        CHECK(dp::encoded_size_of(sampleValue) == sample.encoded_length);
    }
    SECTION("with decode")
    {
        simple_test_input_stream inputStream(sample.encoded_bytes());

        std::vector<std::byte> value;
        REQUIRE(dp::decode(inputStream, value));

        CHECK_BLOB_EQ(value, sample.value);
    }
}

TEST_CASE("std::vector has a codec")
{
    item_sample_rt<std::vector<int>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<int>>("arrays.yaml",
                                                                "int arrays"));
    INFO(sample);

    SECTION("with encode")
    {
        simple_test_output_stream outputStream(sample.encoded.size());

        REQUIRE(dp::encode(outputStream, sample.value));

        CHECK_BLOB_EQ(outputStream.written(), sample.encoded_bytes());
    }
    SECTION("with size_of")
    {
        CHECK(dp::encoded_size_of(sample.value) == sample.encoded.size());
    }
    SECTION("with decode")
    {
        simple_test_input_stream inputStream(sample.encoded);

        std::remove_reference_t<decltype(sample.value)> value;
        REQUIRE(dp::decode(inputStream, value));

        CHECK(std::ranges::equal(value, sample.value));
    }
}

TEST_CASE("std::set has a codec")
{
    item_sample_ct<std::set<int>> const sample{
            .value = {0x01, 0x02, 0x03, 0x0a},
            .encoded_length = 5,
            .encoded = { 0x84, 0x01, 0x02, 0x03,0x0a},
    };

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

        std::remove_cvref_t<decltype(sample.value)> value;
        REQUIRE(dp::decode(inputStream, value));

        CHECK(std::ranges::equal(value, sample.value));
    }
}

TEST_CASE("std::map has a codec")
{
    item_sample_rt<std::vector<std::pair<int, int>>> const sample
            = GENERATE(load_samples_from_yaml<std::vector<std::pair<int, int>>>(
                    "maps.yaml", "int maps"));
    INFO(sample);
    std::map<int, int> const sampleValue(sample.value.begin(),
                                         sample.value.end());

    SECTION("with encode")
    {
        simple_test_output_stream outputStream(sample.encoded.size());

        REQUIRE(dp::encode(outputStream, sampleValue));

        CHECK_BLOB_EQ(outputStream.written(), sample.encoded_bytes());
    }
    SECTION("with size_of")
    {
        CHECK(dp::encoded_size_of(sampleValue) == sample.encoded.size());
    }
    SECTION("with decode")
    {
        simple_test_input_stream inputStream(sample.encoded);

        std::remove_cvref_t<decltype(sampleValue)> value;
        REQUIRE(dp::decode(inputStream, value));

        CHECK(std::ranges::equal(value, sampleValue));
    }
}

TEST_CASE("std::span of bytes has a codec")
{
    item_sample_ct<std::array<std::byte, 2U>> const sample{
            .value = cncr::make_byte_array<2U>({0xfe, 0xfe}
              ),
            .encoded_length = 3,
            .encoded = { 0x42, 0xfe,0xfe},
    };
    dp::bytes const sampleValue = std::span(sample.value);

    SECTION("with encode")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        REQUIRE(dp::encode(outputStream, sampleValue));

        CHECK_BLOB_EQ(outputStream.written(), sample.encoded_bytes());
    }
    SECTION("with size_of")
    {
        CHECK(dp::encoded_size_of(sampleValue) == sample.encoded_length);
    }
    SECTION("with decode")
    {
        simple_test_input_stream inputStream(sample.encoded_bytes());

        std::vector<std::byte> memory(sample.value.size());
        std::span<std::byte> value(memory);
        REQUIRE(dp::decode(inputStream, value));

        CHECK_BLOB_EQ(value, sampleValue);
    }
}

TEST_CASE("std::array of bytes has a codec")
{
    item_sample_ct<std::array<std::byte, 2U>> const sample{
            .value = cncr::make_byte_array<2U>({0xfe, 0xfe}
              ),
            .encoded_length = 3,
            .encoded = { 0x42, 0xfe,0xfe},
    };

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

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        std::array<std::byte, 2U> value;
        REQUIRE(dp::decode(inputStream, value));

        CHECK_BLOB_EQ(value, sample.value);
    }
    SECTION("which rejects undersized items")
    {
        simple_test_input_stream inputStream(sample.encoded_bytes());

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        std::array<std::byte, 3U> value;
        CHECK(dp::decode(inputStream, value).error()
              == dp::errc::tuple_size_mismatch);
    }
}

TEST_CASE("a fixed size std-array has a codec")
{
    item_sample_ct<std::array<unsigned char, 2U>> const sample{
            .value = {0xfe, 0xfe },
            .encoded_length = 5,
            .encoded = { 0x82, 0x18, 0xfe, 0x18, 0xfe},
    };

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

        std::remove_reference_t<decltype(sample.value)> value;
        REQUIRE(dp::decode(inputStream, value));

        CHECK(std::ranges::equal(value, sample.value));
    }
}

TEST_CASE("an indefinite range gets encoded")
{
    item_sample_ct<std::array<unsigned char const, 2U>> const sample{
            .value = {0xfe, 0xfe },
            .encoded_length = 6,
            .encoded = { 0x9f, 0x18, 0xfe, 0x18, 0xfe, 0xff},
    };

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);

    REQUIRE(dp::encode(outputStream, dp::indefinite_range(sample.value)));

    CHECK(std::ranges::equal(buffer, sample.encoded_bytes()));
}

/*
TEST_CASE("a custom associative range gets encoded")
{
    item_sample_ct<std::array<std::pair<int, unsigned>, 2>> const sample{
            .value = {{{0x04, 0x02}, {0x01, 0x03}}},
            .encoded_length = 5,
            .encoded = { 0xa2, 0x04, 0x2, 0x01, 0x03},
    };

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);

    REQUIRE(dp::encode(outputStream, sample.value));

    CHECK(std::ranges::equal(buffer, sample.encoded_bytes()));
}

TEST_CASE("a custom indefinite associative range gets encoded")
{
    item_sample_ct<std::array<std::pair<int, unsigned>, 2>> const sample{
            .value = {{{0x04, 0x02}, {0x01, 0x03}}},
            .encoded_length = 6,
            .encoded = { 0xbf, 0x04, 0x2, 0x01, 0x03, 0xff},
    };

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);

    REQUIRE(dp::encode(outputStream, dp::indefinite_range(sample.value)));

    CHECK(std::ranges::equal(buffer, sample.encoded_bytes()));
}
*/

} // namespace dp_tests

// NOLINTEND(readability-function-cognitive-complexity)
