
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/std-chrono.hpp"

#include <array>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <dplx/cncr/mp_lite.hpp>

#include "blob_matcher.hpp"
#include "dplx/dp/api.hpp"
#include "item_sample_ct.hpp"
#include "test_input_stream.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

// NOLINTBEGIN(readability-function-cognitive-complexity)

namespace dp_tests
{

using duration_types = dplx::cncr::mp_list<std::chrono::nanoseconds,
                                           std::chrono::microseconds,
                                           std::chrono::milliseconds,
                                           std::chrono::seconds,
                                           std::chrono::minutes,
                                           std::chrono::hours,
                                           std::chrono::days,
                                           std::chrono::weeks,
                                           std::chrono::months,
                                           std::chrono::years>;

TEMPLATE_LIST_TEST_CASE("duration values have a codec", "", duration_types)
{
    constexpr item_sample_ct<int> sample{
            0xfe, 2, {0x18, 0xFE}
    };

    SECTION("with encode")
    {
        simple_test_output_stream outStream(sample.encoded_length);

        SECTION("from lvalues")
        {
            TestType const subject(sample.value);
            REQUIRE(dp::encode(outStream, subject));

            CHECK_BLOB_EQ(outStream.written(), sample.encoded_bytes());
        }
        SECTION("from rvalues")
        {
            REQUIRE(dp::encode(outStream, TestType(sample.value)));

            CHECK_BLOB_EQ(outStream.written(), sample.encoded_bytes());
        }
    }
    SECTION("with size_of")
    {
        CHECK(dp::encoded_size_of(TestType(sample.value))
              == sample.encoded_length);
    }
    SECTION("with decode")
    {
        simple_test_input_stream inStream(sample.encoded_bytes());

        TestType value; // NOLINT(cppcoreguidelines-pro-type-member-init)
        REQUIRE(dp::decode(inStream, value));

        CHECK(value == TestType(sample.value));
        CHECK(inStream.discarded() == sample.encoded_length);
    }
}

} // namespace dp_tests

// NOLINTEND(readability-function-cognitive-complexity)
