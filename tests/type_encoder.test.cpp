
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/type_encoder.hpp>

#include <boost/mp11.hpp>

#include "boost-test.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

template dplx::dp::type_encoder<dp_tests::test_output_stream<>>;

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(type_encoder)

struct misc_encoding_fixture
{
    test_output_stream<> encodingBuffer{};
};

BOOST_FIXTURE_TEST_SUITE(misc, misc_encoding_fixture)

using test_encoder = dplx::dp::type_encoder<test_output_stream<>>;

#pragma region Appendix A.Examples

template <typename T>
struct value_sample
{
    T value;
    std::array<std::byte, sizeof(T)> encoded_value;
};

template <typename T>
auto boost_test_print_type(std::ostream &s, value_sample<T> const &sample)
    -> std::ostream &
{
    fmt::print(s, "value_sample{{.value={}, .encoded={{", sample.value);
    for (auto b : sample.encoded_value)
    {
        fmt::print(s, "{:2x}", static_cast<std::uint8_t>(b));
    }
    fmt::print(s, "}}");

    return s;
}

constexpr value_sample<float> float_single_samples[] = {
    {100000.0f, make_byte_array(0x47, 0xc3, 0x50, 0x00)},
    {3.4028234663852886e+38f, make_byte_array(0x7f, 0x7f, 0xff, 0xff)},
    {100000.0f, make_byte_array(0x47, 0xc3, 0x50, 0x00)},
    {std::numeric_limits<float>::infinity(),
     make_byte_array(0x7f, 0x80, 0x00, 0x00)},
    {std::numeric_limits<float>::quiet_NaN(),
     make_byte_array(0x7f, 0xc0, 0x00, 0x00)},
    {-std::numeric_limits<float>::infinity(),
     make_byte_array(0xff, 0x80, 0x00, 0x00)}};

BOOST_DATA_TEST_CASE(float_single,
                     boost::unit_test::data::make(float_single_samples))
{
    test_encoder::float_single(encodingBuffer, sample.value);

    BOOST_TEST_REQUIRE(encodingBuffer.size() ==
                       sample.encoded_value.size() + 1);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::float_single);

    BOOST_TEST(std::span(encodingBuffer).subspan(1) == sample.encoded_value,
               boost::test_tools::per_element{});
}

constexpr value_sample<double> float_double_samples[] = {
    {1.1, make_byte_array(0x3f, 0xf1, 0x99, 0x99, 0x99, 0x99, 0x99, 0x9a)},
    {1.e+300, make_byte_array(0x7e, 0x37, 0xe4, 0x3c, 0x88, 0x00, 0x75, 0x9c)},
    {std::numeric_limits<double>::infinity(),
     make_byte_array(0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)},
    {std::numeric_limits<double>::quiet_NaN(),
     make_byte_array(0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)},
    {-std::numeric_limits<double>::infinity(),
     make_byte_array(0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)}};


BOOST_DATA_TEST_CASE(float_double,
                     boost::unit_test::data::make(float_double_samples))
{
    test_encoder::float_double(encodingBuffer, sample.value);

    BOOST_TEST_REQUIRE(encodingBuffer.size() ==
                       sample.encoded_value.size() + 1);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::float_double);

    BOOST_TEST(std::span(encodingBuffer).subspan(1) == sample.encoded_value,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(bool_false)
{
    test_encoder::boolean(encodingBuffer, false);

    BOOST_TEST(encodingBuffer.size() == 1);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0b111'10100});
}

BOOST_AUTO_TEST_CASE(bool_true)
{
    test_encoder::boolean(encodingBuffer, true);

    BOOST_TEST(encodingBuffer.size() == 1);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0b111'10101});
}

BOOST_AUTO_TEST_CASE(null)
{
    test_encoder::null(encodingBuffer);

    BOOST_TEST(encodingBuffer.size() == 1);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0b111'10110});
}

BOOST_AUTO_TEST_CASE(undefined)
{
    test_encoder::undefined(encodingBuffer);

    BOOST_TEST(encodingBuffer.size() == 1);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0b111'10111});
}

BOOST_AUTO_TEST_CASE(stop)
{
    test_encoder::stop(encodingBuffer);

    BOOST_TEST(encodingBuffer.size() == 1);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0xff});
}

#pragma endregion

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
