
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/encoder/std_path.hpp"

#include <string_view>

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/encoder/core.hpp>

#include "boost-test.hpp"
#include "encoder.test_utils.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(encoder)

BOOST_FIXTURE_TEST_SUITE(std_path, default_encoding_fixture)

static_assert(dp::encodable<std::filesystem::path, test_output_stream<>>);
static_assert(
        cncr::tag_invocable<dp::encoded_size_of_fn, std::filesystem::path>);

struct path_sample
{
    std::size_t prefix_length;
    std::array<std::byte, 8> expected_prefix;
    std::u8string_view content;

    friend inline auto boost_test_print_type(std::ostream &s,
                                             path_sample const &sample)
            -> std::ostream &
    {
        std::string_view charView(
                reinterpret_cast<char const *>(sample.content.data()),
                sample.content.size());

        fmt::print(s, "string_sample{{.content={}, .expected_prefix={{",
                   charView);
        for (auto b : std::span<std::byte const>(sample.expected_prefix)
                              .first(sample.prefix_length))
        {
            fmt::print(s, "{:2x}", static_cast<std::uint8_t>(b));
        }
        fmt::print(s, "}}");

        return s;
    }
};

using namespace std::string_view_literals;

constexpr path_sample path_samples[] = {
        {1,make_byte_array<8>({0b011'00000}),                                                 u8""sv},
        { 1,
         make_byte_array<8, int>({0b011'00000 | 15, u8'f', u8'o', u8'o', u8'/',
 u8'b', u8'a', u8'r'}),                   u8"foo/bar/baz.txt"sv}
};

BOOST_DATA_TEST_CASE(path_with, boost::unit_test::data::make(path_samples))
{
    using test_type = std::filesystem::path;
    using test_encoder = dp::basic_encoder<test_type, test_output_stream<>>;

    test_type path(sample.content);
    DPLX_TEST_RESULT(test_encoder()(encodingBuffer, path));

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
