
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/encoder/core.hpp>
#include <dplx/dp/indefinite_range.hpp>

#include <array>
#include <list>
#include <vector>

#include "boost-test.hpp"
#include "encoder.test_utils.hpp"

namespace dp_tests
{

BOOST_FIXTURE_TEST_SUITE(encoder, default_encoding_fixture)

BOOST_AUTO_TEST_SUITE(range)

static_assert(std::ranges::range<dplx::dp::indefinite_range<int *>>);
static_assert(std::ranges::view<dplx::dp::indefinite_range<int *>>);

static_assert(dplx::dp::encodable<test_output_stream<>,
                                   std::array<simple_encodeable, 5>>);
static_assert(
    dplx::dp::encodable<test_output_stream<>, std::list<simple_encodeable>>);
static_assert(
    dplx::dp::encodable<test_output_stream<>, std::vector<simple_encodeable>>);

struct range_sample
{
    std::size_t num;
    std::size_t prefix_length;
    std::array<std::byte, 8> expected_prefix;
};

auto boost_test_print_type(std::ostream &s, range_sample const &sample)
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

constexpr range_sample vector_samples[] = {
    {0, 1, make_byte_array(0b100'00000, 0, 0, 0, 0, 0, 0, 0)},
    {1, 1, make_byte_array(0b100'00001, 1, 0, 0, 0, 0, 0, 0)},
    {23, 1, make_byte_array(0b100'00000 | 23, 1, 2, 3, 4, 5, 6, 7)},
    {24, 2, make_byte_array(0b100'00000 | 24, 24, 1, 2, 3, 4, 5, 6)},
    {255, 2, make_byte_array(0b100'00000 | 24, 255, 1, 2, 3, 4, 5, 6)},
    {256, 3, make_byte_array(0b100'00000 | 25, 1, 0, 1, 2, 3, 4, 5)}};

BOOST_DATA_TEST_CASE(vector_with, boost::unit_test::data::make(vector_samples))
{
    using test_vector = std::vector<simple_encodeable>;
    using test_stream = test_output_stream<512>;
    using test_encoder = dplx::dp::basic_encoder<test_stream, test_vector>;
    using prefix_span = std::span<std::byte const>;

    std::vector<std::byte> bytes{};
    test_vector vs{};
    bytes.reserve(sample.num);
    vs.reserve(sample.num);
    for (std::size_t i = 0; i < sample.num; ++i)
    {
        std::byte v = static_cast<std::byte>(i + 1);
        bytes.push_back(v);
        vs.push_back(simple_encodeable{v});
    }

    test_stream ctx{};
    DPLX_TEST_RESULT(test_encoder{ctx}(vs));

    BOOST_TEST(
        prefix_span(ctx).first(sample.prefix_length) ==
            prefix_span(sample.expected_prefix).first(sample.prefix_length),
        boost::test_tools::per_element{});
    BOOST_TEST(prefix_span(ctx).subspan(sample.prefix_length) ==
                   prefix_span(bytes),
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(c_array_2)
{
    auto encoded =
        make_byte_array(to_byte(dplx::dp::type_code::array) | std::byte{2},
                        std::byte{0b1110'1001},
                        std::byte{0b1110'1001});

    simple_encodeable const values[] = {{encoded[1]}, {encoded[2]}};

    using test_encoder =
        dplx::dp::basic_encoder<test_output_stream<>, simple_encodeable[2]>;

    DPLX_TEST_RESULT(test_encoder{encodingBuffer}(values));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(std_array_2)
{
    auto encoded =
        make_byte_array(to_byte(dplx::dp::type_code::array) | std::byte{2},
                        std::byte{0b1110'1001},
                        std::byte{0b1110'1001});

    std::array<simple_encodeable const, 2> const values{
        simple_encodeable{encoded[1]}, simple_encodeable{encoded[2]}};

    using test_encoder =
        dplx::dp::basic_encoder<test_output_stream<>,
                                std::array<simple_encodeable const, 2>>;

    DPLX_TEST_RESULT(test_encoder{encodingBuffer}(values));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(std_span_2)
{
    auto encoded =
        make_byte_array(to_byte(dplx::dp::type_code::array) | std::byte{2},
                        std::byte{0b1110'1001},
                        std::byte{0b1110'1001});

    std::array const values{simple_encodeable{encoded[1]},
                            simple_encodeable{encoded[2]}};

    std::span values_span(values);

    using test_encoder =
        dplx::dp::basic_encoder<test_output_stream<>,
                                std::span<simple_encodeable const, 2>>;

    DPLX_TEST_RESULT(test_encoder{encodingBuffer}(values_span));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(indefinite_range_2)
{
    auto encoded =
        make_byte_array(to_byte(dplx::dp::type_code::array) | std::byte{31},
                        std::byte{0b1110'1001},
                        std::byte{0b1110'1001},
                        std::byte{0xFF});

    std::array<simple_encodeable, 2> const values{
        simple_encodeable{encoded[1]}, simple_encodeable{encoded[2]}};

    dplx::dp::indefinite_range indefiniteRange(values);

    using test_encoder = dplx::dp::basic_encoder<test_output_stream<>,
                                                 decltype(indefiniteRange)>;

    DPLX_TEST_RESULT(test_encoder{encodingBuffer}(indefiniteRange));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}

template <std::input_iterator T>
class unsized_input_view : public std::ranges::view_base
{
    T mIt;
    T mEnd;

public:
    constexpr explicit unsized_input_view(T it, T endIt)
        : mIt(std::move(it))
        , mEnd(std::move(endIt))
    {
    }
    unsized_input_view() noexcept = default;

    constexpr auto begin() const -> T
    {
        return mIt;
    }
    constexpr auto end() const -> T
    {
        return mEnd;
    }
};
BOOST_AUTO_TEST_CASE(unsized_input_view_2)
{
    auto encoded =
        make_byte_array(to_byte(dplx::dp::type_code::array) | std::byte{2},
                        std::byte{0b1110'1001},
                        std::byte{0b1110'1001});

    std::list<simple_encodeable> const values{simple_encodeable{encoded[1]},
                                              simple_encodeable{encoded[2]}};

    unsized_input_view unsizedInputView(values.begin(), values.end());

    using view_type = decltype(unsizedInputView);
    static_assert(!dplx::dp::enable_indefinite_encoding<view_type>);
    static_assert(!std::ranges::sized_range<view_type>);

    using test_encoder =
        dplx::dp::basic_encoder<test_output_stream<>, view_type>;

    DPLX_TEST_RESULT(test_encoder{encodingBuffer}(unsizedInputView));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
