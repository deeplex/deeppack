
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/decoder/chrono.hpp>

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/decoder/api.hpp>

#include "boost-test.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(decoder)

BOOST_AUTO_TEST_SUITE(chrono)

static_assert(
        dp::decodable<std::chrono::system_clock::duration, test_input_stream>);

BOOST_AUTO_TEST_CASE(duration_ref)
{
    auto bytes = make_byte_array<32>({0x18, 0xFE});
    test_input_stream instream(bytes);

    std::chrono::steady_clock::duration out;
    auto rx = dp::decode(instream, out);

    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out == std::chrono::steady_clock::duration{0xFE});
}

BOOST_AUTO_TEST_CASE(duration_value)
{
    auto bytes = make_byte_array<32>({0x18, 0xFE});
    test_input_stream instream(bytes);

    auto rx = dp::decode(dp::as_value<std::chrono::steady_clock::duration>,
                         instream);

    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(rx.assume_value() == std::chrono::steady_clock::duration{0xFE});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
