
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

#include "dplx/dp/api.hpp"
#include "dplx/dp/streams/memory_output_stream2.hpp"
#include "item_sample.hpp"
#include "test_utils.hpp"

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
    constexpr item_sample<int> sample{
            0xfe, 2, {0x18, 0xFE}
    };

    SECTION("with encode")
    {
        std::vector<std::byte> buffer(sample.encoded_length);
        dp::memory_output_stream out(buffer);

        SECTION("from lvalues")
        {
            TestType const subject(sample.value);
            REQUIRE(dp::encode(out, subject));
        }
        SECTION("from rvalues")
        {
            REQUIRE(dp::encode(out, TestType(sample.value)));
        }

        CHECK(std::ranges::equal(buffer, sample.encoded_bytes()));
    }
    SECTION("with size_of")
    {
        CHECK(dp::encoded_size_of(TestType(sample.value))
              == sample.encoded_length);
    }
}

} // namespace dp_tests
