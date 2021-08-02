
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/encoder/core.hpp>

#include <tuple>

#include "boost-test.hpp"
#include "encoder.test_utils.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(encoder)

BOOST_FIXTURE_TEST_SUITE(tuple, default_encoding_fixture)

BOOST_AUTO_TEST_CASE(std_pair)
{
    auto encoded
            = make_byte_array(to_byte(dp::type_code::array) | std::byte{2},
                              std::byte{0b1110'1000}, std::byte{0b1110'1001});

    using value_type = std::pair<simple_encodeable, simple_encodeable>;
    value_type values = {{encoded[1]}, {encoded[2]}};

    using test_encoder = dp::basic_encoder<value_type, test_output_stream<>>;

    DPLX_TEST_RESULT(test_encoder()(encodingBuffer, values));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(std_tuple_2)
{
    auto encoded
            = make_byte_array(to_byte(dp::type_code::array) | std::byte{2},
                              std::byte{0b1110'1000}, std::byte{0b1110'1001});

    using value_type = std::tuple<simple_encodeable, simple_encodeable>;
    value_type values = {{encoded[1]}, {encoded[2]}};

    using test_encoder = dp::basic_encoder<value_type, test_output_stream<>>;

    DPLX_TEST_RESULT(test_encoder()(encodingBuffer, values));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(std_tuple_3)
{
    auto encoded
            = make_byte_array(to_byte(dp::type_code::array) | std::byte{3},
                              std::byte{0b1110'1000}, std::byte{0b0000'1001},
                              std::byte{0b1110'1010});

    using value_type
            = std::tuple<simple_encodeable, unsigned int, simple_encodeable>;
    value_type values = {
            {encoded[1]}, static_cast<unsigned int>(encoded[2]), {encoded[3]}};

    using test_encoder = dp::basic_encoder<value_type, test_output_stream<>>;

    DPLX_TEST_RESULT(test_encoder()(encodingBuffer, values));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
