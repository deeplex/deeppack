
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/std-filesystem.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "blob_matcher.hpp"
#include "dplx/dp/api.hpp"
#include "item_sample_ct.hpp"
#include "range_generator.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

namespace
{
using namespace std::string_view_literals;
constexpr item_sample_ct<std::u8string_view> path_samples[] = {
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

    SECTION("to a stream")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        REQUIRE(dp::encode(outputStream, value));

        CHECK_BLOB_EQ(outputStream.written(), sample.encoded_bytes());
    }
    SECTION("with a size_of operator")
    {
        CHECK(dp::encoded_size_of(value) == sample.encoded_length);
    }
}

} // namespace dp_tests
