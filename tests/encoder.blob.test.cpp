
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/encoder/core.hpp>

#include <span>
#include <vector>

#include "boost-test.hpp"
#include "encoder.test_utils.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(encoder)

BOOST_FIXTURE_TEST_SUITE(blob, default_encoding_fixture)

static_assert(dp::encodable<std::vector<std::byte>, test_output_stream<>>);
static_assert(dp::encodable<std::span<std::byte>, test_output_stream<>>);

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
