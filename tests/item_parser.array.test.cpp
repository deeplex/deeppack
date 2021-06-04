
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

BOOST_AUTO_TEST_SUITE(array)

using parse = dp::item_parser<test_input_stream>;

BOOST_AUTO_TEST_CASE(std_vector_simple)
{
    using container_type = std::vector<std::byte>;

    auto const memoryData
            = make_byte_array<32>({0x85, 0x01, 0x02, 0x03, 0x04, 0x05});
    test_input_stream stream(memoryData);

    int callbackInvocations = 0;

    std::vector<std::byte> out{};
    auto parseRx = parse::array(
            stream, out,
            [&callbackInvocations](
                    test_input_stream &inStream, container_type &container,
                    std::size_t const i) noexcept -> dp::result<void>
            {
                callbackInvocations += 1;

                container.resize(i + 1);

                DPLX_TRY(dp::read(inStream, container.data() + i, 1));

                return dp::oc::success();
            });

    DPLX_REQUIRE_RESULT(parseRx);
    BOOST_TEST(parseRx.assume_value() == 5u);
    BOOST_TEST(callbackInvocations == 5);
    auto expected = make_byte_array<5>({0x01, 0x02, 0x03, 0x04, 0x05},
                                       std::byte{0x00});
    BOOST_TEST(out == expected, boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(std_vector_indefinite)
{
    using container_type = std::vector<std::byte>;

    auto const memoryData
            = make_byte_array<32>({0x9f, 0xc1, 0xc2, 0xc7, 0xc4, 0xc5, 0xff});
    test_input_stream stream(memoryData);

    int callbackInvocations = 0;

    std::vector<std::byte> out{};
    auto parseRx = parse::array(
            stream, out,
            [&callbackInvocations](
                    test_input_stream &inStream, container_type &container,
                    std::size_t const i) noexcept -> dp::result<void>
            {
                callbackInvocations += 1;

                container.resize(i + 1);

                DPLX_TRY(dp::read(inStream, container.data() + i, 1));

                return dp::oc::success();
            });

    DPLX_REQUIRE_RESULT(parseRx);
    BOOST_TEST(parseRx.assume_value() == 5u);
    BOOST_TEST(callbackInvocations == 5);
    auto expected = make_byte_array<5>({0xc1, 0xc2, 0xc7, 0xc4, 0xc5},
                                       std::byte{0x00});
    BOOST_TEST(out == expected, boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(std_vector_finite)
{
    using container_type = std::vector<std::byte>;

    auto const memoryData
            = make_byte_array<32>({0x85, 0x01, 0x02, 0x03, 0x04, 0x05});
    test_input_stream stream(memoryData);

    int callbackInvocations = 0;

    std::vector<std::byte> out{};
    auto parseRx = parse::array_finite(
            stream, out,
            [&callbackInvocations](
                    test_input_stream &inStream, container_type &container,
                    std::size_t const i) noexcept -> dp::result<void>
            {
                callbackInvocations += 1;

                container.resize(i + 1);

                DPLX_TRY(dp::read(inStream, container.data() + i, 1));

                return dp::oc::success();
            });

    DPLX_REQUIRE_RESULT(parseRx);
    BOOST_TEST(parseRx.assume_value() == 5u);
    BOOST_TEST(callbackInvocations == 5);
    auto expected = make_byte_array<5>({0x01, 0x02, 0x03, 0x04, 0x05},
                                       std::byte{0x00});
    BOOST_TEST(out == expected, boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
