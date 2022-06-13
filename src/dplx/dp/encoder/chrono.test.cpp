
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/encoder/chrono.hpp"

#include "boost-test.hpp"
#include "encoder.test_utils.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(encoder)

BOOST_FIXTURE_TEST_SUITE(chrono, default_encoding_fixture)

static_assert(dp::encodable<std::chrono::system_clock::duration,
                            test_output_stream<>>);

BOOST_AUTO_TEST_CASE(duration_lvalue)
{
    auto encoded = make_byte_array<2>({0x18, 0xFE});
    std::chrono::steady_clock::duration testValue{0xFE};

    DPLX_REQUIRE_RESULT(dp::encode(encodingBuffer, testValue));

    BOOST_TEST(encodingBuffer.size() == 2);
    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(duration_rvalue)
{
    auto encoded = make_byte_array<2>({0x18, 0xFE});

    DPLX_REQUIRE_RESULT(dp::encode(encodingBuffer,
                                   std::chrono::steady_clock::duration{0xFE}));

    BOOST_TEST(encodingBuffer.size() == 2);
    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(duration_size_of)
{
    BOOST_TEST(dp::encoded_size_of(std::chrono::steady_clock::duration{0xFE})
               == 2U);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
