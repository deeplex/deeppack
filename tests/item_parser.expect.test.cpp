
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

BOOST_AUTO_TEST_SUITE(expect)

namespace bdata = boost::unit_test::data;

using parse = dp::item_parser<test_input_stream>;

struct acceptance_sample
{
    dp::type_code type;
    std::uint64_t value;
    dp::parse_mode mode;
    std::array<std::byte, 16> input;
};

auto boost_test_print_type(std::ostream &s, acceptance_sample const &sample)
        -> std::ostream &
{
    fmt::print(s, "acceptance_sample{{.type={}, .value={}, .mode={}, .input=0x",
               sample.type, sample.value, static_cast<int>(sample.mode));
    for (auto c : sample.input)
    {
        fmt::print(s, "{:02x}", c);
    }
    return s << "}";
}

constexpr acceptance_sample acceptance_samples[] = {
        {dp::type_code::posint, 0u, dp::parse_mode::strict,
         make_byte_array<16>({0x00})},
        {dp::type_code::negint, 24u, dp::parse_mode::strict,
         make_byte_array<16>({0x38, 0x18})},
        {dp::type_code::special, 0x1fu, dp::parse_mode::strict,
         make_byte_array<16>({0xff})},
        {dp::type_code::posint, 23u, dp::parse_mode::lenient,
         make_byte_array<16>({0x18, 0x17})},
};

BOOST_DATA_TEST_CASE(accepts_valid_input, bdata::make(acceptance_samples))
{
    test_input_stream stream(sample.input);
    auto expectRx
            = parse::expect(stream, sample.type, sample.value, sample.mode);

    DPLX_REQUIRE_RESULT(expectRx);
}

struct rejection_sample
{
    dp::errc code;
    dp::type_code type;
    std::uint64_t value;
    dp::parse_mode mode;
    std::array<std::byte, 16> input;
};

auto boost_test_print_type(std::ostream &s, rejection_sample const &sample)
        -> std::ostream &
{
    fmt::print(s,
               "rejection_samples{{.code={}, .type={}, .value={}, .mode={}, "
               ".input=0x",
               sample.code, sample.type, sample.value,
               static_cast<int>(sample.mode));
    for (auto c : sample.input)
    {
        fmt::print(s, "{:02x}", c);
    }
    return s << "}";
}

constexpr rejection_sample rejection_samples[] = {
        {dp::errc::item_type_mismatch, dp::type_code::special, 0u,
         dp::parse_mode::strict, make_byte_array<16>({0x00})},
        {dp::errc::item_type_mismatch, dp::type_code::special, 22u,
         dp::parse_mode::strict, make_byte_array<16>({0xff})},
        {dp::errc::invalid_additional_information, dp::type_code::negint, 24u,
         dp::parse_mode::strict, make_byte_array<16>({0x3f, 0x18})},
        {dp::errc::indefinite_item, dp::type_code::binary, 21u,
         dp::parse_mode::strict, make_byte_array<16>({0x5f})},
        {dp::errc::item_value_out_of_range, dp::type_code::array, 21u,
         dp::parse_mode::strict, make_byte_array<16>({0x82})},
        {dp::errc::oversized_additional_information_coding,
         dp::type_code::posint, 23u, dp::parse_mode::strict,
         make_byte_array<16>({0x18, 0x17})},
        {dp::errc::oversized_additional_information_coding,
         dp::type_code::posint, 23u, dp::parse_mode::canonical,
         make_byte_array<16>({0x18, 0x17})},
};

BOOST_DATA_TEST_CASE(rejects_mismatching_input, bdata::make(rejection_samples))
{
    test_input_stream stream(sample.input);
    auto expectRx
            = parse::expect(stream, sample.type, sample.value, sample.mode);

    BOOST_TEST_REQUIRE(expectRx.has_error());
    BOOST_TEST(expectRx.error() == sample.code);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
