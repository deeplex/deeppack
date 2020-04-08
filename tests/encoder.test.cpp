
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/encoder/core.hpp>

#include "boost-test.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_FIXTURE_TEST_SUITE(encoder, default_encoding_fixture)

static_assert(!dplx::dp::encodeable<test_output_stream<>, volatile int>);
static_assert(!dplx::dp::encodeable<test_output_stream<>, volatile int const>);
static_assert(!dplx::dp::encodeable<test_output_stream<>, char>);

// the integer encoder template just forwards to typ_encoder::integer()
// which is already covered by the type_encoder test suite
static_assert(dplx::dp::encodeable<test_output_stream<>, signed char>);
static_assert(dplx::dp::encodeable<test_output_stream<>, short>);
static_assert(dplx::dp::encodeable<test_output_stream<>, int>);
static_assert(dplx::dp::encodeable<test_output_stream<>, long>);
static_assert(dplx::dp::encodeable<test_output_stream<>, long long>);

static_assert(dplx::dp::encodeable<test_output_stream<>, unsigned char>);
static_assert(dplx::dp::encodeable<test_output_stream<>, unsigned short>);
static_assert(dplx::dp::encodeable<test_output_stream<>, unsigned int>);
static_assert(dplx::dp::encodeable<test_output_stream<>, unsigned long>);
static_assert(dplx::dp::encodeable<test_output_stream<>, unsigned long long>);


BOOST_AUTO_TEST_CASE(bool_false)
{
    using test_encoder = dplx::dp::basic_encoder<test_output_stream<>, bool>;
    test_encoder{encodingBuffer}(false);

    BOOST_TEST(encodingBuffer.size() == 1);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::bool_false);
}
BOOST_AUTO_TEST_CASE(bool_true)
{
    using test_encoder = dplx::dp::basic_encoder<test_output_stream<>, bool>;
    test_encoder{encodingBuffer}(true);

    BOOST_TEST(encodingBuffer.size() == 1);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::bool_true);
}

BOOST_AUTO_TEST_CASE(null_value)
{
    using test_encoder =
        dplx::dp::basic_encoder<test_output_stream<>, dplx::dp::null_type>;
    test_encoder{encodingBuffer}(dplx::dp::null_value);

    BOOST_TEST(encodingBuffer.size() == 1);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::null);
}

BOOST_AUTO_TEST_CASE(float_api)
{
    using test_encoder = dplx::dp::basic_encoder<test_output_stream<>, float>;
    test_encoder{encodingBuffer}(100000.0f);

    auto encodedValue = make_byte_array(0x47, 0xc3, 0x50, 0x00);
    BOOST_TEST_REQUIRE(encodingBuffer.size() == encodedValue.size() + 1);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::float_single);

    BOOST_TEST(std::span(encodingBuffer).subspan(1) == encodedValue,
               boost::test_tools::per_element{});
}
BOOST_AUTO_TEST_CASE(double_api)
{
    using test_encoder = dplx::dp::basic_encoder<test_output_stream<>, double>;
    test_encoder{encodingBuffer}(1.1);

    auto encodedValue =
        make_byte_array(0x3f, 0xf1, 0x99, 0x99, 0x99, 0x99, 0x99, 0x9a);
    BOOST_TEST_REQUIRE(encodingBuffer.size() == encodedValue.size() + 1);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::float_double);

    BOOST_TEST(std::span(encodingBuffer).subspan(1) == encodedValue,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(void_dispatch_api)
{
    using test_encoder = dplx::dp::basic_encoder<test_output_stream<>, void>;
    test_encoder{encodingBuffer}(dplx::dp::null_value);

    BOOST_TEST(encodingBuffer.size() == 1);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::null);
}

BOOST_AUTO_TEST_CASE(vararg_dispatch_0)
{
    using test_encoder =
        dplx::dp::basic_encoder<test_output_stream<>, dplx::dp::mp_varargs<>>;
    test_encoder{encodingBuffer}();

    BOOST_TEST(encodingBuffer.size() == 1);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::array);
}
BOOST_AUTO_TEST_CASE(vararg_dispatch_1)
{
    using test_encoder =
        dplx::dp::basic_encoder<test_output_stream<>,
                                dplx::dp::mp_varargs<dplx::dp::null_type>>;
    test_encoder{encodingBuffer}(dplx::dp::null_value);

    BOOST_TEST(encodingBuffer.size() == 2);
    BOOST_TEST(encodingBuffer.data()[0] == (to_byte(dplx::dp::type_code::array) |
               std::byte{1}));
    BOOST_TEST(encodingBuffer.data()[1] == dplx::dp::type_code::null);
}
BOOST_AUTO_TEST_CASE(vararg_dispatch_2)
{
    using test_encoder =
        dplx::dp::basic_encoder<test_output_stream<>,
                                dplx::dp::mp_varargs<dplx::dp::null_type, int>>;
    test_encoder{encodingBuffer}(dplx::dp::null_value, 0);

    BOOST_TEST(encodingBuffer.size() == 3);
    BOOST_TEST(encodingBuffer.data()[0] == (to_byte(dplx::dp::type_code::array) |
               std::byte{2}));
    BOOST_TEST(encodingBuffer.data()[1] == dplx::dp::type_code::null);
    BOOST_TEST(encodingBuffer.data()[2] == dplx::dp::type_code::posint);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
