
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/tuple.hpp"

#include <catch2/catch_test_macros.hpp>

#include "dplx/dp/codecs/core.hpp"
#include "dplx/dp/encoder/api.hpp"
#include "dplx/dp/streams/memory_output_stream2.hpp"
#include "item_sample.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

static_assert(dp::ng::encodable<std::pair<int, long>>);

TEST_CASE("encodes two tuples")
{
    constexpr item_sample<std::array<int, 2>> sample{
            {   3, 22},
            3, { 0x82,  3,  22}
    };

    std::vector<std::byte> encodingBuffer(sample.encoded_length);
    dp::memory_output_stream outputStream(encodingBuffer);

    SECTION("with std::pair")
    {
        std::pair<int, long long> subject{sample.value[0], sample.value[1]};
        REQUIRE(dp::encode(outputStream, subject));
    }
    SECTION("with std::tuple")
    {
        std::tuple<int, long long> subject{sample.value[0], sample.value[1]};
        REQUIRE(dp::encode(outputStream, subject));
    }

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

TEST_CASE("encodes three tuples")
{
    constexpr item_sample<std::array<int, 3>> sample{
            {   3, 22,  5},
            4, { 0x83,  3, 22,   5}
    };

    std::vector<std::byte> encodingBuffer(sample.encoded_length);
    dp::memory_output_stream outputStream(encodingBuffer);

    std::tuple<int, long long, long> subject{sample.value[0], sample.value[1],
                                             sample.value[2]};
    REQUIRE(dp::encode(outputStream, subject));

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

} // namespace dp_tests
