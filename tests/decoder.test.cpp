
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/decoder/api.hpp>
#include <dplx/dp/decoder/core.hpp>

#include "boost-test.hpp"
#include "test_utils.hpp"
#include "test_input_stream.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(decoder)

BOOST_AUTO_TEST_SUITE(api)

BOOST_AUTO_TEST_CASE(int_decode)
{
    auto bytes = make_byte_array<32>({0x16});
    test_input_stream istream(bytes);

    int out;
    auto rx = dplx::dp::decode(istream, out);

    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out == 0x16);
}

BOOST_AUTO_TEST_CASE(int_decode_value)
{
    auto bytes = make_byte_array<32>({0x16});
    test_input_stream istream(bytes);

    auto rx = dplx::dp::decode(dplx::dp::as_value<int>, istream);

    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(rx.assume_value() == 0x16);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

}
