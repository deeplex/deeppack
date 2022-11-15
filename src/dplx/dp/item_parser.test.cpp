
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/item_parser.hpp"

#include <dplx/cncr/misc.hpp>

#include <dplx/dp/skip_item.hpp>

#include "boost-test.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)

template <>
struct fmt::formatter<dplx::dp::item_info>
{
    static constexpr auto parse(format_parse_context &ctx)
            -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatCtx>
    auto format(dplx::dp::item_info const &info, FormatCtx &ctx)
            -> decltype(ctx.out())
    {
        return fmt::format_to(
                ctx.out(),
                "{{type={:#04x}, .flags={:#04x} .encoded_length={}, "
                ".value={:#018x}}}",
                dplx::cncr::to_underlying(info.type),
                dplx::cncr::to_underlying(info.flags), info.encoded_length,
                info.value);
    }
};

namespace dplx::dp
{
auto boost_test_print_type(std::ostream &s, item_info const &sample)
        -> std::ostream &
{
    fmt::print(s, "item_info{{{}}}", sample);
    return s;
}
inline auto boost_test_print_type(std::ostream &s, item_info::flag flags)
        -> std::ostream &
{
    fmt::print(s, "{:2x}", static_cast<std::uint8_t>(flags));
    return s;
}
} // namespace dplx::dp

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(item_parser)

using dp::item_info;
using dp::type_code;

using parse = dp::item_parser<dp_tests::test_input_stream>;

struct parse_sample
{
    std::array<std::byte, 9> stream;
    item_info expected;
};

auto boost_test_print_type(std::ostream &s, parse_sample const &sample)
        -> std::ostream &
{
    fmt::print(s, "parse_sample{{.stream={{}}, .expected={}}}",
               sample.expected);
    return s;
}

constexpr parse_sample parse_samples[] = {

  // Appendix A.Examples

        {                                              make_byte_array<9>({0x00}),{type_code::posint, {}, 1, 0}                                                                                  },
        {                                              make_byte_array<9>({0x01}),    {type_code::posint, {}, 1, 1}},
        {                                              make_byte_array<9>({0x0a}), {type_code::posint, {}, 1, 0x0a}},
        {                                        make_byte_array<9>({0x18, 0x19}), {type_code::posint, {}, 2, 0x19}},
        {                                        make_byte_array<9>({0x18, 0x64}), {type_code::posint, {}, 2, 0x64}},
        {                                  make_byte_array<9>({0x19, 0x03, 0xe8}),
         {type_code::posint, {}, 3, 0x03e8}                                                                        },
        {                      make_byte_array<9>({0x1a, 0x00, 0x0f, 0x42, 0x40}),
         {type_code::posint, {}, 5, 0x000f'4240}                                                                   },
        {make_byte_array<9>(
{0x1b, 0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00}),
         {type_code::posint, {}, 9, 0x0000'00e8'd4a5'1000}                                                         },

        {                                              make_byte_array<9>({0x29}),    {type_code::negint, {}, 1, 9}},
        {                                        make_byte_array<9>({0x38, 0x63}),   {type_code::negint, {}, 2, 99}},
        {                                  make_byte_array<9>({0x39, 0x03, 0xe7}),
         {type_code::negint, {}, 3, 999}                                                                           },

 // posint
        {                                              make_byte_array<9>({0x17}), {type_code::posint, {}, 1, 0x17}},
        {                                        make_byte_array<9>({0x18, 0x18}), {type_code::posint, {}, 2, 0x18}},
        {                                        make_byte_array<9>({0x18, 0xff}), {type_code::posint, {}, 2, 0xff}},
        {                                  make_byte_array<9>({0x19, 0x01, 0x00}),
         {type_code::posint, {}, 3, 0x0100}                                                                        },
        {                                  make_byte_array<9>({0x19, 0xff, 0xff}),
         {type_code::posint, {}, 3, 0xffff}                                                                        },
        {                      make_byte_array<9>({0x1a, 0x00, 0x01, 0x00, 0x00}),
         {type_code::posint, {}, 5, 0x1'0000}                                                                      },
        {                      make_byte_array<9>({0x1a, 0xff, 0xff, 0xff, 0xff}),
         {type_code::posint, {}, 5, 0xffff'ffff}                                                                   },
        {make_byte_array<9>(
{0x1b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00}),
         {type_code::posint, {}, 9, 0x1'0000'0000}                                                                 },
        {make_byte_array<9>(
{0x1b, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}),
         {type_code::posint, {}, 9, 0xffff'ffff'ffff'ffff}                                                         },
        {                                              make_byte_array<9>({0xff}),
         {type_code::special, item_info::flag::indefinite, 1, 0x1f}                                                },
};

BOOST_DATA_TEST_CASE(parse_speculative,
                     boost::unit_test::data::make(parse_samples))
{
    test_input_stream stream(std::span(sample.stream));
    auto parseRx = parse::generic(stream);
    DPLX_REQUIRE_RESULT(parseRx);

    item_info const &parsed = parseRx.assume_value();
    item_info const &expected = sample.expected;
    BOOST_TEST(parsed.type == expected.type);
    BOOST_TEST(parsed.flags == expected.flags);
    BOOST_TEST(parsed.encoded_length == expected.encoded_length);
    BOOST_TEST(parsed.value == expected.value);
}

BOOST_DATA_TEST_CASE(parse_safe, boost::unit_test::data::make(parse_samples))
{
    test_input_stream stream(std::span<std::byte const>(sample.stream)
                                     .first(sample.expected.encoded_length));
    auto parseRx = parse::generic(stream);
    DPLX_REQUIRE_RESULT(parseRx);

    item_info const &parsed = parseRx.assume_value();
    item_info const &expected = sample.expected;
    BOOST_TEST(parsed.type == expected.type);
    BOOST_TEST(parsed.flags == expected.flags);
    BOOST_TEST(parsed.encoded_length == expected.encoded_length);
    BOOST_TEST(parsed.value == expected.value);
}

BOOST_DATA_TEST_CASE(skip_simple, boost::unit_test::data::make(parse_samples))
{
    if (sample.stream[0] == type_code::special_break)
    {
        return;
    }
    test_input_stream stream(std::span<std::byte const>(sample.stream)
                                     .first(sample.expected.encoded_length));
    auto parseRx = dp::skip_item(stream);
    DPLX_REQUIRE_RESULT(parseRx);

    auto remainingRx = dp::available_input_size(stream);
    BOOST_TEST(remainingRx.value() == 0U);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
