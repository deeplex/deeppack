
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/core.hpp"

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <dplx/cncr/misc.hpp>

#include "dplx/dp/encoder/api.hpp"
#include "dplx/dp/streams/memory_output_stream2.hpp"
#include "item_sample.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

TEMPLATE_TEST_CASE("integers are encodable",
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
    static_assert(dp::ng::encodable<TestType>);
    item_sample<unsigned char> const sample{
            0x7e, 2, {0x18, 0x7e}
    };

    std::vector<std::byte> encodingBuffer(sample.encoded_length);
    dp::memory_output_stream outputStream(encodingBuffer);

    REQUIRE(dp::encode(outputStream, static_cast<TestType>(sample.value)));

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

TEST_CASE("bool is encodable")
{
    static_assert(dp::ng::encodable<bool>);
    item_sample<bool> const sample{true, 1, {0xf5}};

    std::vector<std::byte> encodingBuffer(sample.encoded_length);
    dp::memory_output_stream outputStream(encodingBuffer);

    REQUIRE(dp::encode(outputStream, sample.value));

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

TEST_CASE("float is encodable")
{
    static_assert(dp::ng::encodable<float>);
    item_sample<float> const sample{
            100000.0F, 5, {0xfa, 0x47, 0xc3, 0x50, 0x00}
    };
    std::vector<std::byte> encodingBuffer(sample.encoded_length);
    dp::memory_output_stream outputStream(encodingBuffer);

    REQUIRE(dp::encode(outputStream, sample.value));

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

TEST_CASE("double is encodable")
{
    static_assert(dp::ng::encodable<double>);
    item_sample<double> const sample{
            1.e+300, 9, {0xfb, 0x7e, 0x37, 0xe4, 0x3c, 0x88, 0x00, 0x75, 0x9c}
    };
    std::vector<std::byte> encodingBuffer(sample.encoded_length);
    dp::memory_output_stream outputStream(encodingBuffer);

    REQUIRE(dp::encode(outputStream, sample.value));

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

} // namespace dp_tests
