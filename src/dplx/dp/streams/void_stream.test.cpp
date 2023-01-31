
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/streams/void_stream.hpp"

#include <catch2/catch_test_macros.hpp>

#include "test_utils.hpp"

namespace dp_tests
{

TEST_CASE("void_stream can be default constructed")
{
    dp::void_stream subject;
    // ensure it is an output_buffer
    dp::output_buffer &out = subject;

    SECTION("which isn't empty by default")
    {
        CHECK(!out.empty());
        CHECK(out.size() > 0U); // NOLINT(readability-container-size-empty)
        CHECK(out.data() != nullptr);
        CHECK(out.begin() != out.end());
    }
    SECTION("offers more space than the minimum_guaranteed_write_size")
    {
        CHECK(out.ensure_size(dp::minimum_guaranteed_write_size));
    }
    SECTION("include used current buffer size in the total count")
    {
        out.commit_written(1U);
        CHECK(subject.total_written() == 1U);
    }
}

} // namespace dp_tests
