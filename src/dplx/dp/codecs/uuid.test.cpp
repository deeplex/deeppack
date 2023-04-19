
// Copyright 2023 Henrik Steffen Gaßmann
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/uuid.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <dplx/dp/api.hpp>

#include "blob_matcher.hpp"
#include "item_sample_ct.hpp"
#include "range_generator.hpp"
#include "test_input_stream.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

namespace
{

using namespace cncr::uuid_literals;

constexpr item_sample_ct<cncr::uuid, 20U> uuid_samples[] = {
        {"00000000-0000-0000-0000-000000000000"_uuid,
         17, {0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
        {"00000000-0000-4000-bfff-ffffffffffff"_uuid,
         17, {0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0xbf, 0xff,
 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
        {"ffffffff-ffff-ffff-ffff-ffffffffffff"_uuid,
         17, {0x50, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
};

} // namespace

TEST_CASE("cncr::uuid has a codec")
{
    auto const sample = GENERATE(borrowed_range(uuid_samples));
    INFO(sample);

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

        cncr::uuid value{};
        REQUIRE(dp::decode(inputStream, value));

        CHECK(value == sample.value);
    }
}

} // namespace dp_tests
