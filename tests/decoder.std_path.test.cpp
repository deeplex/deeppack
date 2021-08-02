
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/decoder/std_path.hpp>

#include "boost-test.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/decoder/core.hpp>

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(decoder)

BOOST_AUTO_TEST_SUITE(std_path)

static_assert(dp::decodable<std::filesystem::path, test_input_stream>);

using test_encoder
        = dp::basic_decoder<std::filesystem::path, test_input_stream>;

using namespace std::string_view_literals;

struct path_sample
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
                                             path_sample const &c)
            -> std::ostream &
    {
        std::string_view conv{reinterpret_cast<char const *>(c.expected.data()),
                              c.expected.size()};
        return s << conv;
    }
};

constexpr path_sample path_samples[]
        = {{u8""sv, 1, make_byte_array<12>({0x60})},
           {u8"foo/bar/baz.txt"sv, 1, make_byte_array<12>({0x60 | 15})}};

BOOST_DATA_TEST_CASE(path_with, boost::unit_test::data::make(path_samples))
{
    auto const sampleBytes = sample.derive_input_vector();
    test_input_stream sampleStream{std::span(sampleBytes)};

    std::filesystem::path out;
    auto rx = test_encoder{}(sampleStream, out);
    DPLX_REQUIRE_RESULT(rx);

    BOOST_TEST(sample.expected == std::u8string_view(out.generic_u8string()),
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
