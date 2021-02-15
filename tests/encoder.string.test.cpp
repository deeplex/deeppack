
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/encoder/core.hpp>

#include <string>
#include <string_view>

#include <boost/unordered_map.hpp>

#include "boost-test.hpp"
#include "encoder.test_utils.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_FIXTURE_TEST_SUITE(encoder, default_encoding_fixture)

BOOST_AUTO_TEST_SUITE(string)

using namespace std::string_view_literals;

static_assert(dplx::dp::encodable<std::u8string, test_output_stream<>>);
static_assert(dplx::dp::encodable<std::u8string_view, test_output_stream<>>);

struct string_sample
{
    std::size_t prefix_length;
    std::array<std::byte, 8> expected_prefix;
    std::u8string_view content;
};

auto boost_test_print_type(std::ostream &s, string_sample const &sample)
        -> std::ostream &
{
    std::string_view charView(
            reinterpret_cast<char const *>(sample.content.data()),
            sample.content.size());

    fmt::print(s, "string_sample{{.content={}, .expected_prefix={{", charView);
    for (auto b : std::span(sample.expected_prefix).first(sample.prefix_length))
    {
        fmt::print(s, "{:2x}", static_cast<std::uint8_t>(b));
    }
    fmt::print(s, "}}");

    return s;
}

constexpr string_sample string_samples[]
        = {{1, make_byte_array<8>({0b011'00000}), u8""sv},
           {1,
            make_byte_array<8, int>({0b011'00000 | 11, u8'h', u8'e', u8'l',
                                     u8'l', u8'o', u8' ', u8'w'}),
            u8"hello world"sv}};

BOOST_DATA_TEST_CASE(string_view_with,
                     boost::unit_test::data::make(string_samples))
{
    using test_type = std::u8string_view;
    using test_encoder
            = dplx::dp::basic_encoder<test_type, test_output_stream<>>;

    DPLX_TEST_RESULT(test_encoder()(encodingBuffer, sample.content));

    BOOST_TEST(byte_span(encodingBuffer).first(sample.prefix_length)
                       == byte_span(encodingBuffer).first(sample.prefix_length),
               boost::test_tools::per_element{});

    BOOST_TEST(byte_span(encodingBuffer).subspan(sample.prefix_length)
                       == as_bytes(std::span(sample.content)),
               boost::test_tools::per_element{});
}

BOOST_DATA_TEST_CASE(string_with, boost::unit_test::data::make(string_samples))
{
    using test_type = std::u8string;
    using test_encoder
            = dplx::dp::basic_encoder<test_type, test_output_stream<>>;

    test_type content(sample.content);
    DPLX_TEST_RESULT(test_encoder()(encodingBuffer, content));

    BOOST_TEST(byte_span(encodingBuffer).first(sample.prefix_length)
                       == byte_span(encodingBuffer).first(sample.prefix_length),
               boost::test_tools::per_element{});

    BOOST_TEST(byte_span(encodingBuffer).subspan(sample.prefix_length)
                       == as_bytes(std::span(sample.content)),
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
