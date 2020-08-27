
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/decoder/std_string.hpp>

#include "boost-test.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(decoder)

struct std_string_dependencies
{
    dplx::dp::basic_decoder<std::u8string, test_input_stream> subject;
    std::u8string out;
};

BOOST_FIXTURE_TEST_SUITE(std_string, std_string_dependencies)

using test_encoder = dplx::dp::basic_decoder<std::u8string, test_input_stream>;

using namespace std::string_view_literals;

struct string_sample
{
    std::u8string_view expected;
    unsigned int prefix_length;
    std::array<std::byte, 12> expected_prefix;

    auto derive_input_vector() const -> std::vector<std::byte>
    {
        std::vector<std::byte> v;
        v.resize(expected.size() + expected_prefix.size());
        std::memcpy(v.data(), expected_prefix.data(), prefix_length);
        std::memcpy(v.data() + prefix_length, expected.data(), expected.size());
        return v;
    }
    friend inline auto boost_test_print_type(std::ostream &s,
                                             string_sample const &c)
        -> std::ostream &
    {
        std::string_view conv{reinterpret_cast<char const *>(c.expected.data()),
                              c.expected.size()};
        s << conv;
        return s;
    }
};

constexpr string_sample rfc_samples[] = {
    {u8""sv, 1, make_byte_array<12>({0x60})},
    {u8"a"sv, 1, make_byte_array<12>({0x61})},
    {u8"IETF"sv, 1, make_byte_array<12>({0x64})},
    {u8"\"\\"sv, 1, make_byte_array<12>({0x62})},
    {u8"\u00fc"sv, 1, make_byte_array<12>({0x62})},
    {u8"\u6c34"sv, 1, make_byte_array<12>({0x63})},
    {u8"\U00010151"sv, 1, make_byte_array<12>({0x64})}};

BOOST_DATA_TEST_CASE(rfc_sample, boost::unit_test::data::make(rfc_samples))
{
    auto const sampleBytes = sample.derive_input_vector();
    test_input_stream sampleStream{std::span(sampleBytes)};

    auto rx = subject(sampleStream, out);
    DPLX_REQUIRE_RESULT(rx);

    BOOST_TEST(sample.expected == std::u8string_view(out),
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(handles_indefinite_strings)
{
    auto const expected = u8"tis nothing but a string"sv;
    auto sampleBytes = make_byte_array<28>(
        {0x7f, 0x6A, 0x74, 0x69, 0x73, 0x20, 0x6E, 0x6F, 0x74, 0x68,
         0x69, 0x6e, 0x6E, 0x67, 0x20, 0x62, 0x75, 0x74, 0x20, 0x61,
         0x20, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0xFF});
    test_input_stream sampleStream{std::span(sampleBytes)};

    DPLX_REQUIRE_RESULT(subject(sampleStream, out));

    BOOST_TEST(expected == std::u8string_view(out),
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(rejects_undersized_string_0)
{
    auto const sampleBytes = make_byte_array<1>({0x6f});
    test_input_stream sampleStream{std::span(sampleBytes)};

    auto rx = subject(sampleStream, out);
    BOOST_TEST_REQUIRE(rx.has_error());
    BOOST_TEST(rx.assume_error() == dplx::dp::errc::missing_data);
    BOOST_TEST(out.size() == 0u);
}

BOOST_AUTO_TEST_CASE(rejects_undersized_string_1)
{
    auto const sampleBytes = make_byte_array<3>({0x7f, 0x78, 0xff});
    test_input_stream sampleStream{std::span(sampleBytes)};

    auto rx = subject(sampleStream, out);
    BOOST_TEST_REQUIRE(rx.has_error());
    BOOST_TEST(rx.assume_error() == dplx::dp::errc::missing_data);
    BOOST_TEST(out.size() == 0u);
}

BOOST_AUTO_TEST_CASE(rejects_other_item_type)
{
    auto const sampleBytes = make_byte_array<1>({0x00});
    test_input_stream sampleStream{std::span(sampleBytes)};

    auto rx = subject(sampleStream, out);
    BOOST_TEST_REQUIRE(rx.has_error());
    BOOST_TEST(rx.assume_error() == dplx::dp::errc::item_type_mismatch);
    BOOST_TEST(out.size() == 0u);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
