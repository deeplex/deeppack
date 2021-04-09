
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/item_parser.hpp>
#include <dplx/dp/skip_item.hpp>

#include "boost-test.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"

namespace fmt
{
template <>
struct formatter<dplx::dp::detail::item_info>
{
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatCtx>
    auto format(dplx::dp::detail::item_info const &info, FormatCtx &ctx)
    {
        return fmt::format_to(ctx.out(),
                              "{{.code={}, .type={:#04x}, .encoded_length={}, "
                              ".value={:#018x}}}",
                              info.code, info.type, info.encoded_length,
                              info.value);
    }
};
} // namespace fmt

namespace dplx::dp::detail
{
auto boost_test_print_type(std::ostream &s, item_info const &sample)
        -> std::ostream &
{
    fmt::print(s, "item_info{{{}}}", sample);
    return s;
}
auto boost_test_print_type(std::ostream &s, decode_errc const code)
        -> std::ostream &
{
    switch (code)
    {
    case decode_errc::nothing:
        return s << "decode_errc::nothing";
    case decode_errc::invalid_additional_information:
        return s << "decode_errc::invalid_additional_information";
    default:
        return s << "{invalid decode_errc value: " << static_cast<int>(code)
                 << "}";
    }
}
} // namespace dplx::dp::detail

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(item_parser)

using dplx::dp::detail::decode_errc;
using dplx::dp::detail::item_info;

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

        {make_byte_array<9>({0x00}), {decode_errc::nothing, 0, 1, 0}},
        {make_byte_array<9>({0x01}), {decode_errc::nothing, 0, 1, 1}},
        {make_byte_array<9>({0x0a}), {decode_errc::nothing, 0, 1, 0x0a}},
        {make_byte_array<9>({0x18, 0x19}), {decode_errc::nothing, 0, 2, 0x19}},
        {make_byte_array<9>({0x18, 0x64}), {decode_errc::nothing, 0, 2, 0x64}},
        {make_byte_array<9>({0x19, 0x03, 0xe8}),
         {decode_errc::nothing, 0, 3, 0x03e8}},
        {make_byte_array<9>({0x1a, 0x00, 0x0f, 0x42, 0x40}),
         {decode_errc::nothing, 0, 5, 0x000f'4240}},
        {make_byte_array<9>(
                 {0x1b, 0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00}),
         {decode_errc::nothing, 0, 9, 0x0000'00e8'd4a5'1000}},

        {make_byte_array<9>({0x29}), {decode_errc::nothing, 0x20, 1, 9}},
        {make_byte_array<9>({0x38, 0x63}), {decode_errc::nothing, 0x20, 2, 99}},
        {make_byte_array<9>({0x39, 0x03, 0xe7}),
         {decode_errc::nothing, 0x20, 3, 999}},

        // posint
};

BOOST_DATA_TEST_CASE(parse_speculative,
                     boost::unit_test::data::make(parse_samples))
{
    test_input_stream stream(std::span(sample.stream));
    auto parseRx = dplx::dp::detail::parse_item_info(stream);
    DPLX_REQUIRE_RESULT(parseRx);

    item_info const &parsed = parseRx.assume_value();
    item_info const &expected = sample.expected;
    BOOST_TEST(parsed.code == expected.code);
    BOOST_TEST(parsed.type == expected.type);
    BOOST_TEST(parsed.encoded_length == expected.encoded_length);
    BOOST_TEST(parsed.value == expected.value);
}

BOOST_DATA_TEST_CASE(parse_safe, boost::unit_test::data::make(parse_samples))
{
    test_input_stream stream(std::span<std::byte const>(sample.stream)
                                     .first(static_cast<std::size_t>(
                                             sample.expected.encoded_length)));
    auto parseRx = dplx::dp::detail::parse_item_info(stream);
    DPLX_REQUIRE_RESULT(parseRx);

    item_info const &parsed = parseRx.assume_value();
    item_info const &expected = sample.expected;
    BOOST_TEST(parsed.code == expected.code);
    BOOST_TEST(parsed.type == expected.type);
    BOOST_TEST(parsed.encoded_length == expected.encoded_length);
    BOOST_TEST(parsed.value == expected.value);
}

BOOST_DATA_TEST_CASE(skip_simple, boost::unit_test::data::make(parse_samples))
{
    if (sample.expected.code != dplx::dp::detail::decode_errc::nothing)
    {
        return;
    }

    test_input_stream stream(std::span<std::byte const>(sample.stream)
                                     .first(static_cast<std::size_t>(
                                             sample.expected.encoded_length)));
    auto parseRx = dplx::dp::skip_item(stream);
    DPLX_REQUIRE_RESULT(parseRx);

    auto remainingRx = dplx::dp::available_input_size(stream);
    BOOST_TEST(remainingRx.value() == 0u);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
