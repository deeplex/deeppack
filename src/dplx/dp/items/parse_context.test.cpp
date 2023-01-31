
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/parse_context.hpp"

#include <catch2/catch_test_macros.hpp>

#include "test_input_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

TEST_CASE("parse_context can be instantiated given an input_buffer reference")
{
    simple_test_input_stream stream({});
    [[maybe_unused]] dp::parse_context ctx{
            static_cast<dp::input_buffer &>(stream)};
}

} // namespace dp_tests
