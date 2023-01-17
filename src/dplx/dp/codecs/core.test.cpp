
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/core.hpp"

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <dplx/cncr/misc.hpp>

#include "dplx/dp/api.hpp"
#include "dplx/dp/streams/memory_output_stream2.hpp"
#include "item_sample_ct.hpp"
#include "test_input_stream.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

TEMPLATE_TEST_CASE("integers have a codec",
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
    static_assert(dp::ng::decodable<TestType>);
    static_assert(dp::ng::encodable<TestType>);
    auto const sample = item_sample_ct<unsigned char>{
            0x7e,
            2,
            {0x18, 0x7e}
    }.as<TestType>();

    SECTION("with encode")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        REQUIRE(dp::encode(outputStream, static_cast<TestType>(sample.value)));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }
    SECTION("with size_of")
    {
        CHECK(dp::encoded_size_of(sample.value) == sample.encoded_length);
    }
    SECTION("with decode")
    {
        simple_test_input_stream inputStream(sample.encoded_bytes());

        TestType decoded;
        REQUIRE(dp::decode(inputStream, decoded));

        CHECK(decoded == sample.value);
        CHECK(inputStream.discarded() == sample.encoded_length);
    }
}

TEST_CASE("bool is encodable")
{
    static_assert(dp::ng::encodable<bool>);
    item_sample_ct<bool> const sample{true, 1, {0xf5}};

    SECTION("with encode")
    {
        std::vector<std::byte> encodingBuffer(sample.encoded_length);
        dp::memory_output_stream outputStream(encodingBuffer);

        REQUIRE(dp::encode(outputStream, sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }
    SECTION("with size_of")
    {
        CHECK(dp::encoded_size_of(sample.value) == sample.encoded_length);
    }
}

TEST_CASE("float has a codec")
{
    static_assert(dp::ng::encodable<float>);
    item_sample_ct<float> const sample{
            100000.0F, 5, {0xfa, 0x47, 0xc3, 0x50, 0x00}
    };

    SECTION("with encode")
    {
        std::vector<std::byte> encodingBuffer(sample.encoded_length);
        dp::memory_output_stream outputStream(encodingBuffer);

        REQUIRE(dp::encode(outputStream, sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }
    SECTION("with size_of")
    {
        CHECK(dp::encoded_size_of(sample.value) == sample.encoded_length);
    }
}

TEST_CASE("double has a codec")
{
    static_assert(dp::ng::encodable<double>);
    item_sample_ct<double> const sample{
            1.e+300, 9, {0xfb, 0x7e, 0x37, 0xe4, 0x3c, 0x88, 0x00, 0x75, 0x9c}
    };

    SECTION("with encode")
    {
        std::vector<std::byte> encodingBuffer(sample.encoded_length);
        dp::memory_output_stream outputStream(encodingBuffer);

        REQUIRE(dp::encode(outputStream, sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }
    SECTION("with size_of")
    {
        CHECK(dp::encoded_size_of(sample.value) == sample.encoded_length);
    }
}

} // namespace dp_tests
