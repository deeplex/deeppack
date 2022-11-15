
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <map>
#include <unordered_map>

#include <boost/unordered_map.hpp>

#include <dplx/dp/encoder/core.hpp>
#include <dplx/dp/indefinite_range.hpp>

#include "boost-test.hpp"
#include "encoder.test_utils.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(encoder)

BOOST_FIXTURE_TEST_SUITE(map, default_encoding_fixture)

static_assert(
        dp::encodable<std::map<int, simple_encodeable>, test_output_stream<>>);
static_assert(dp::encodable<std::unordered_map<int, simple_encodeable>,
                            test_output_stream<>>);

struct map_sample
{
    std::size_t num;
    std::size_t prefix_length;
    std::array<std::byte, 8> expected_prefix;
};

auto boost_test_print_type(std::ostream &s, map_sample const &sample)
        -> std::ostream &
{
    fmt::print(s, "value_sample{{.num={}, .expected_prefix={{", sample.num);
    for (auto b : sample.expected_prefix)
    {
        fmt::print(s, "{:2x}", static_cast<std::uint8_t>(b));
    }
    fmt::print(s, "}}");

    return s;
}

constexpr map_sample map_samples[] = {
        {  0, 1,      make_byte_array(0b101'00000,   0, 0, 0, 0, 0, 0, 0)},
        {  1, 3,      make_byte_array(0b101'00001,   0, 1, 0, 0, 0, 0, 0)},
        { 23, 8, make_byte_array(0b101'00000 | 23,   0, 1, 1, 2, 2, 3, 3)},
        { 24, 8, make_byte_array(0b101'00000 | 24,  24, 0, 1, 1, 2, 2, 3)},
        {255, 8, make_byte_array(0b101'00000 | 24, 255, 0, 1, 1, 2, 2, 3)},
        {256, 8, make_byte_array(0b101'00000 | 25,   1, 0, 0, 1, 1, 2, 2)}
};

BOOST_DATA_TEST_CASE(std_map_with, boost::unit_test::data::make(map_samples))
{
    using test_map = std::map<std::size_t, simple_encodeable>;
    using test_stream = test_output_stream<1024>;
    using test_encoder = dp::basic_encoder<test_map, test_stream>;
    using prefix_span = std::span<std::byte const>;

    test_map vs{};
    for (std::size_t i = 0; i < sample.num; ++i)
    {
        auto const v = static_cast<std::byte>(i + 1);
        vs.insert({i, simple_encodeable{v}});
    }

    test_stream ctx{};
    DPLX_TEST_RESULT(test_encoder()(ctx, vs));

    BOOST_TEST(prefix_span(ctx).first(sample.prefix_length)
                       == prefix_span(sample.expected_prefix)
                                  .first(sample.prefix_length),
               boost::test_tools::per_element{});
}

BOOST_DATA_TEST_CASE(std_unordered_map_with,
                     boost::unit_test::data::make(map_samples))
{
    using test_map = std::unordered_map<std::size_t, simple_encodeable>;
    using test_stream = test_output_stream<1024>;
    using test_encoder = dp::basic_encoder<test_map, test_stream>;
    // using prefix_span = std::span<std::byte const>;

    test_map vs{};
    for (std::size_t i = 0; i < sample.num; ++i)
    {
        auto const v = static_cast<std::byte>(i + 1);
        vs.insert({i, simple_encodeable{v}});
    }

    test_stream ctx{};
    DPLX_TEST_RESULT(test_encoder()(ctx, vs));

    // BOOST_TEST(
    //    prefix_span(ctx).first(sample.prefix_length) ==
    //        prefix_span(sample.expected_prefix).first(sample.prefix_length),
    //    boost::test_tools::per_element{});
}

BOOST_DATA_TEST_CASE(boost_unordered_map_with,
                     boost::unit_test::data::make(map_samples))
{
    using test_map = std::unordered_map<std::size_t, simple_encodeable>;
    using test_stream = test_output_stream<1024>;
    using test_encoder = dp::basic_encoder<test_map, test_stream>;
    // using prefix_span = std::span<std::byte const>;

    test_map vs{};
    for (std::size_t i = 0; i < sample.num; ++i)
    {
        auto const v = static_cast<std::byte>(i + 1);
        vs.insert({i, simple_encodeable{v}});
    }

    test_stream ctx{};
    DPLX_TEST_RESULT(test_encoder()(ctx, vs));

    // BOOST_TEST(
    //    prefix_span(ctx).first(sample.prefix_length) ==
    //        prefix_span(sample.expected_prefix).first(sample.prefix_length),
    //    boost::test_tools::per_element{});
}

BOOST_DATA_TEST_CASE(map_pair_range_with,
                     boost::unit_test::data::make(map_samples))
{
    using test_map = std::vector<dp::map_pair<std::size_t, simple_encodeable>>;
    using test_stream = test_output_stream<1024>;
    using test_encoder = dp::basic_encoder<test_map, test_stream>;
    using prefix_span = std::span<std::byte const>;

    test_map vs{};
    for (std::size_t i = 0; i < sample.num; ++i)
    {
        auto const v = static_cast<std::byte>(i + 1);
        vs.push_back({i, simple_encodeable{v}});
    }

    test_stream ctx{};
    DPLX_TEST_RESULT(test_encoder()(ctx, vs));

    BOOST_TEST(prefix_span(ctx).first(sample.prefix_length)
                       == prefix_span(sample.expected_prefix)
                                  .first(sample.prefix_length),
               boost::test_tools::per_element{});
}

constexpr map_sample indefinite_map_samples[] = {
        {  0, 1, make_byte_array(0b101'11111, 0xFF, 0,    0, 0, 0, 0, 0)},
        {  1, 3, make_byte_array(0b101'11111,    0, 1, 0xFF, 0, 0, 0, 0)},
        { 23, 8, make_byte_array(0b101'11111,    0, 1,    1, 2, 2, 3, 3)},
        { 24, 8, make_byte_array(0b101'11111,    0, 1,    1, 2, 2, 3, 3)},
        {255, 8, make_byte_array(0b101'11111,    0, 1,    1, 2, 2, 3, 3)},
        {256, 8, make_byte_array(0b101'11111,    0, 1,    1, 2, 2, 3, 3)}
};

BOOST_DATA_TEST_CASE(indefinite_map_with,
                     boost::unit_test::data::make(indefinite_map_samples))
{
    using test_map = std::map<std::size_t, simple_encodeable>;
    using test_stream = test_output_stream<1024>;
    using prefix_span = std::span<std::byte const>;

    test_map vs{};
    for (std::size_t i = 0; i < sample.num; ++i)
    {
        auto const v = static_cast<std::byte>(i + 1);
        vs.insert({i, simple_encodeable{v}});
    }

    dp::indefinite_range indefiniteMap(vs);

    using test_encoder
            = dp::basic_encoder<dp::indefinite_range<test_map::const_iterator>,
                                test_stream>;

    test_stream ctx{};
    DPLX_TEST_RESULT(test_encoder()(ctx, indefiniteMap));

    BOOST_TEST(prefix_span(ctx).first(sample.prefix_length)
                       == prefix_span(sample.expected_prefix)
                                  .first(sample.prefix_length),
               boost::test_tools::per_element{});
    BOOST_TEST(prefix_span(ctx).back()
               == to_byte(dp::type_code::special_break));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
