
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/std-filesystem.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "dplx/dp/encoder/api.hpp"
#include "dplx/dp/streams/memory_output_stream2.hpp"
#include "item_sample.hpp"
#include "range_generator.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

namespace
{
using namespace std::string_view_literals;
constexpr item_sample<std::u8string_view> path_samples[] = {
        {
         u8""sv, 1,
         {0x60},
         },
        {
         u8"ba/z.txt"sv, 9,
         {0x68, u8'b', u8'a', u8'/', u8'z', u8'.', u8't', u8'x', u8't'},
         },
};
} // namespace

TEST_CASE("std::filesystem::path can be encoded")
{
    auto const sample = GENERATE(borrowed_range(path_samples));
    std::filesystem::path const value(sample.value);

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);

    REQUIRE(dp::encode(outputStream, value));

    CHECK(std::ranges::equal(buffer, sample.encoded_bytes()));
}

} // namespace dp_tests
