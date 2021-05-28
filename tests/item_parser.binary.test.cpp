
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/item_parser.hpp>

#include <vector>

#include <dplx/dp/customization.std.hpp>

#include "boost-test.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(item_parser)

BOOST_AUTO_TEST_SUITE(binary)

using parse = dp::item_parser<test_input_stream>;

BOOST_AUTO_TEST_CASE(std_array_simple)
{
    auto const memoryData
            = make_byte_array<32>({0x45, 0x01, 0x02, 0x03, 0x04, 0x05});
    test_input_stream stream(memoryData);

    std::array<std::byte, 8> out{};
    auto parseRx = parse::binary(stream, out);

    DPLX_REQUIRE_RESULT(parseRx);
    BOOST_TEST(parseRx.assume_value() == 5);
    auto expected = make_byte_array<8>({0x01, 0x02, 0x03, 0x04, 0x05},
                                       std::byte{0x00});
    BOOST_TEST(out == expected, boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(std_array_indefinite)
{
    auto const memoryData
            = make_byte_array<32>({0x5f, 0x45, 0x01, 0x02, 0x03, 0x04, 0x05,
                                   0x43, 0x06, 0x07, 0x08, 0xff});
    test_input_stream stream(memoryData);

    std::array<std::byte, 8> out{};
    auto parseRx = parse::binary(stream, out, 8u);

    DPLX_REQUIRE_RESULT(parseRx);
    BOOST_TEST(parseRx.assume_value() == 8);
    auto expected = make_byte_array<8>(
            {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}, std::byte{0x00});
    BOOST_TEST(out == expected, boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(std_array_finite)
{
    auto const memoryData
            = make_byte_array<32>({0x45, 0x01, 0x02, 0x03, 0x04, 0x05});
    test_input_stream stream(memoryData);

    std::array<std::byte, 8> out{};
    auto parseRx = parse::binary_finite(stream, out);

    DPLX_REQUIRE_RESULT(parseRx);
    BOOST_TEST(parseRx.assume_value() == 5);
    auto expected = make_byte_array<8>({0x01, 0x02, 0x03, 0x04, 0x05},
                                       std::byte{0x00});
    BOOST_TEST(out == expected, boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(std_vector_simple)
{
    auto const memoryData
            = make_byte_array<32>({0x45, 0x01, 0x02, 0x03, 0x04, 0x05});
    test_input_stream stream(memoryData);

    std::vector<std::byte> out{};
    auto parseRx = parse::binary(stream, out);

    DPLX_REQUIRE_RESULT(parseRx);
    BOOST_TEST(parseRx.assume_value() == 5);
    auto expected = make_byte_array<5>({0x01, 0x02, 0x03, 0x04, 0x05},
                                       std::byte{0x00});
    BOOST_TEST(out == expected, boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(std_vector_indefinite)
{
    auto const memoryData
            = make_byte_array<32>({0x5f, 0x45, 0x01, 0x02, 0x03, 0x04, 0x05,
                                   0x43, 0x06, 0x07, 0x08, 0xff});
    test_input_stream stream(memoryData);

    std::vector<std::byte> out{};
    auto parseRx = parse::binary(stream, out, 8u);

    DPLX_REQUIRE_RESULT(parseRx);
    BOOST_TEST(parseRx.assume_value() == 8);
    auto expected = make_byte_array<8>(
            {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}, std::byte{0x00});
    BOOST_TEST(out == expected, boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(std_vector_finite)
{
    auto const memoryData
            = make_byte_array<32>({0x45, 0x01, 0x02, 0x03, 0x04, 0x05});
    test_input_stream stream(memoryData);

    std::vector<std::byte> out{};
    auto parseRx = parse::binary_finite(stream, out);

    DPLX_REQUIRE_RESULT(parseRx);
    BOOST_TEST(parseRx.assume_value() == 5);
    auto expected = make_byte_array<5>({0x01, 0x02, 0x03, 0x04, 0x05},
                                       std::byte{0x00});
    BOOST_TEST(out == expected, boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
