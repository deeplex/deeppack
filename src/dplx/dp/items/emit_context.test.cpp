
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/emit_context.hpp"

#include <catch2/catch_test_macros.hpp>

#include "dplx/dp/streams/memory_output_stream2.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

TEST_CASE("emit_context can be instantiated given an output_buffer reference")
{
    dp::memory_output_stream stream({});
    [[maybe_unused]] dp::emit_context ctx{
            static_cast<dp::output_buffer &>(stream)};
}

} // namespace dp_tests
