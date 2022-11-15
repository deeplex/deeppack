
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/cncr/mp_lite.hpp>

#include <dplx/dp/item_parser.hpp>

#include "boost-test.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(item_parser)

BOOST_AUTO_TEST_SUITE(integer)

namespace bdata = boost::unit_test::data;

using dp::parse_mode;
using dp::type_code;

using parse = dp::item_parser<dp_tests::test_input_stream>;

using integer_types = cncr::mp_list<signed char,
                                    unsigned char,
                                    short,
                                    unsigned short,
                                    int,
                                    unsigned int,
                                    long,
                                    unsigned long,
                                    long long,
                                    unsigned long long>;
BOOST_AUTO_TEST_CASE_TEMPLATE(compilation, T, integer_types)
{
    auto const memory = make_byte_array<9>({0x00});
    test_input_stream stream(memory);

    dp::result<T> parseRx = parse::integer<T>(stream, parse_mode::canonical);
    DPLX_REQUIRE_RESULT(parseRx);
    BOOST_TEST(parseRx.assume_value() == static_cast<T>(0));
}

constexpr std::array unsigned_type_rejections = {
        type_code::negint,     type_code::binary, type_code::text,
        type_code::array,      type_code::map,    type_code::tag,
        type_code::bool_false, type_code::null,   type_code::undefined,
};

BOOST_DATA_TEST_CASE(unsigned_reject_major_types,
                     bdata::make(unsigned_type_rejections))
{
    auto const memory = make_byte_array<9>({to_byte(sample)});
    test_input_stream stream(memory);

    dp::result<unsigned> parseRx = parse::integer<unsigned>(stream);
    BOOST_TEST(parseRx.has_error());
    BOOST_TEST(parseRx.error() == dp::errc::item_type_mismatch);
}

constexpr std::array signed_type_rejections = {
        type_code::binary, type_code::text,      type_code::array,
        type_code::map,    type_code::tag,       type_code::bool_false,
        type_code::null,   type_code::undefined,
};

BOOST_DATA_TEST_CASE(signed_reject_major_types,
                     bdata::make(signed_type_rejections))
{
    auto const memory = make_byte_array<9>({to_byte(sample)});
    test_input_stream stream(memory);

    dp::result<int> parseRx
            = parse::integer<int>(stream, parse_mode::canonical);
    BOOST_TEST(parseRx.has_error());
    BOOST_TEST(parseRx.error() == dp::errc::item_type_mismatch);
}

BOOST_AUTO_TEST_CASE(int16_max)
{
    auto const memory = make_byte_array<9>({0x19, 0x7f, 0xff});
    test_input_stream stream(memory);

    dp::result<std::int16_t> parseRx
            = parse::integer<std::int16_t>(stream, parse_mode::canonical);
    DPLX_REQUIRE_RESULT(parseRx);
    BOOST_TEST(parseRx.assume_value()
               == std::numeric_limits<std::int16_t>::max());
}
BOOST_AUTO_TEST_CASE(int16_overflow)
{
    auto const memory = make_byte_array<9>({0x19, 0x80, 0x00});
    test_input_stream stream(memory);

    dp::result<std::int16_t> parseRx
            = parse::integer<std::int16_t>(stream, parse_mode::canonical);
    BOOST_TEST(parseRx.has_error());
    BOOST_TEST(parseRx.error() == dp::errc::item_value_out_of_range);
}

BOOST_AUTO_TEST_CASE(int16_min)
{
    auto const memory = make_byte_array<9>({0x39, 0x7f, 0xff});
    test_input_stream stream(memory);

    dp::result<std::int16_t> parseRx
            = parse::integer<std::int16_t>(stream, parse_mode::canonical);
    DPLX_REQUIRE_RESULT(parseRx);
    BOOST_TEST(parseRx.assume_value()
               == std::numeric_limits<std::int16_t>::min());
}
BOOST_AUTO_TEST_CASE(int16_negative_overflow)
{
    auto const memory = make_byte_array<9>({0x39, 0x80, 0x00});
    test_input_stream stream(memory);

    dp::result<std::int16_t> parseRx
            = parse::integer<std::int16_t>(stream, parse_mode::canonical);
    BOOST_TEST(parseRx.has_error());
    BOOST_TEST(parseRx.error() == dp::errc::item_value_out_of_range);
}

BOOST_AUTO_TEST_CASE(uint16_max)
{
    auto const memory = make_byte_array<9>({0x19, 0xff, 0xff});
    test_input_stream stream(memory);

    dp::result<std::uint16_t> parseRx
            = parse::integer<std::uint16_t>(stream, parse_mode::canonical);
    DPLX_REQUIRE_RESULT(parseRx);
    BOOST_TEST(parseRx.assume_value()
               == std::numeric_limits<std::uint16_t>::max());
}
BOOST_AUTO_TEST_CASE(uint16_overflow)
{
    auto const memory = make_byte_array<9>({0x1A, 0x00, 0x01, 0x00, 0x00});
    test_input_stream stream(memory);

    dp::result<std::uint16_t> parseRx
            = parse::integer<std::uint16_t>(stream, parse_mode::canonical);
    BOOST_TEST(parseRx.has_error());
    BOOST_TEST(parseRx.error() == dp::errc::item_value_out_of_range);
}

constexpr std::array oversized_samples = {
        make_byte_array<9>({0x18, 0x17}),
        make_byte_array<9>({0x18, 0x00}),
        make_byte_array<9>({0x19, 0x00, 0xff}),
        make_byte_array<9>({0x19, 0x00, 0x00}),
        make_byte_array<9>({0x1a, 0x00, 0x00, 0xff, 0xff}),
        make_byte_array<9>({0x1a, 0x00, 0x00, 0x00, 0x00}),
        make_byte_array<9>(
                {0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}),
        make_byte_array<9>(
                {0x1b, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff}),
};

BOOST_DATA_TEST_CASE(reject_oversized_items_in_canonical,
                     bdata::make(oversized_samples))
{
    test_input_stream stream(sample);

    auto parseRx = parse::integer<std::uint64_t>(stream, parse_mode::canonical);
    BOOST_TEST(parseRx.has_error());
    BOOST_TEST(parseRx.error()
               == dp::errc::oversized_additional_information_coding);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
