
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <string_view>

#include <boost/unordered_map.hpp>

#include <dplx/predef/compiler.h>

#include <dplx/dp/detail/workaround.hpp>
#include <dplx/dp/encoder/core.hpp>
#include <dplx/dp/encoder/narrow_strings.hpp>

#include "boost-test.hpp"
#include "encoder.test_utils.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

#if DPLX_DP_WORKAROUND(DPLX_COMP_CLANG, <, 15, 0, 0)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function" // std::size_t casts :(
namespace
{

// see https://github.com/llvm/llvm-project/issues/55560
// NOLINTNEXTLINE(clang-diagnostic-unused-function)
auto clang_string_workaround(char8_t const *a, char8_t const *b)
        -> std::u8string
{
    return {a, b};
}

} // namespace
#pragma clang diagnostic pop
#endif

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(encoder)

BOOST_FIXTURE_TEST_SUITE(string, default_encoding_fixture)

using namespace std::string_view_literals;

static_assert(dp::encodable<std::u8string, test_output_stream<>>);
static_assert(dp::encodable<std::u8string_view, test_output_stream<>>);

static_assert(dp::encodable<std::string, test_output_stream<>>);
static_assert(dp::encodable<std::string_view, test_output_stream<>>);

template <typename Char>
struct basic_string_sample
{
    std::size_t prefix_length;
    std::array<std::byte, 8> expected_prefix;
    std::basic_string_view<Char> content;
};
using string_sample = basic_string_sample<char>;
using u8string_sample = basic_string_sample<char8_t>;

template <typename Char>
auto boost_test_print_type(std::ostream &s,
                           basic_string_sample<Char> const &sample)
        -> std::ostream &
{
    std::string_view charView(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<char const *>(sample.content.data()),
            sample.content.size());

    fmt::print(s, "string_sample{{.content={}, .expected_prefix={{", charView);
    for (auto b : std::span<std::byte const>(sample.expected_prefix)
                          .first(sample.prefix_length))
    {
        fmt::print(s, "{:2x}", static_cast<std::uint8_t>(b));
    }
    fmt::print(s, "}}");

    return s;
}

constexpr u8string_sample u8string_samples[] = {
        {1,make_byte_array<8>({0b011'00000}),                                                 u8""sv},
        { 1,
         make_byte_array<8, int>({0b011'00000 | 11, u8'h', u8'e', u8'l', u8'l',
 u8'o', u8' ', u8'w'}),                       u8"hello world"sv}
};

BOOST_DATA_TEST_CASE(u8string_view_with,
                     boost::unit_test::data::make(u8string_samples))
{
    using test_type = std::u8string_view;
    using test_encoder = dp::basic_encoder<test_type, test_output_stream<>>;

    DPLX_TEST_RESULT(test_encoder()(encodingBuffer, sample.content));

    BOOST_TEST(byte_span(encodingBuffer).first(sample.prefix_length)
                       == byte_span(encodingBuffer).first(sample.prefix_length),
               boost::test_tools::per_element{});

    BOOST_TEST(byte_span(encodingBuffer).subspan(sample.prefix_length)
                       == as_bytes(std::span(sample.content)),
               boost::test_tools::per_element{});
}

BOOST_DATA_TEST_CASE(u8string_with,
                     boost::unit_test::data::make(u8string_samples))
{
    using test_type = std::u8string;
    using test_encoder = dp::basic_encoder<test_type, test_output_stream<>>;

    test_type content(sample.content);
    DPLX_TEST_RESULT(test_encoder()(encodingBuffer, content));

    BOOST_TEST(byte_span(encodingBuffer).first(sample.prefix_length)
                       == byte_span(encodingBuffer).first(sample.prefix_length),
               boost::test_tools::per_element{});

    BOOST_TEST(byte_span(encodingBuffer).subspan(sample.prefix_length)
                       == as_bytes(std::span(sample.content)),
               boost::test_tools::per_element{});
}

constexpr string_sample string_samples[] = {
        {1,make_byte_array<8>({0b011'00000}),                                                                   ""sv},
        { 1,
         make_byte_array<8, int>(
 {0b011'00000 | 11, 'h', 'e', 'l', 'l', 'o', ' ', 'w'}),        "hello world"sv}
};

BOOST_DATA_TEST_CASE(string_view_with,
                     boost::unit_test::data::make(string_samples))
{
    using test_type = std::string_view;
    using test_encoder = dp::basic_encoder<test_type, test_output_stream<>>;

    DPLX_TEST_RESULT(test_encoder()(encodingBuffer, sample.content));

    BOOST_TEST(byte_span(encodingBuffer).first(sample.prefix_length)
                       == byte_span(encodingBuffer).first(sample.prefix_length),
               boost::test_tools::per_element{});

    BOOST_TEST(byte_span(encodingBuffer).subspan(sample.prefix_length)
                       == as_bytes(std::span(sample.content)),
               boost::test_tools::per_element{});
}

BOOST_DATA_TEST_CASE(string_view_size_with,
                     boost::unit_test::data::make(string_samples))
{
    BOOST_TEST(dp::encoded_size_of(sample.content)
               == sample.prefix_length + sample.content.size());
}

BOOST_DATA_TEST_CASE(string_with, boost::unit_test::data::make(string_samples))
{
    using test_type = std::string;
    using test_encoder = dp::basic_encoder<test_type, test_output_stream<>>;

    test_type content(sample.content);
    DPLX_TEST_RESULT(test_encoder()(encodingBuffer, content));

    BOOST_TEST(byte_span(encodingBuffer).first(sample.prefix_length)
                       == byte_span(encodingBuffer).first(sample.prefix_length),
               boost::test_tools::per_element{});

    BOOST_TEST(byte_span(encodingBuffer).subspan(sample.prefix_length)
                       == as_bytes(std::span(sample.content)),
               boost::test_tools::per_element{});
}

BOOST_DATA_TEST_CASE(string_size_with,
                     boost::unit_test::data::make(string_samples))
{
    BOOST_TEST(dp::encoded_size_of(std::string{sample.content})
               == sample.prefix_length + sample.content.size());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
