
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/decoder/api.hpp>
#include <dplx/dp/decoder/core.hpp>
#include <dplx/dp/encoder/api.hpp>
#include <dplx/dp/encoder/core.hpp>

#include "boost-test.hpp"
#include "encoder.test_utils.hpp"
#include "test_input_stream.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace
{

enum class signed_test_enum : int
{
    neg2 = -2,
    neg1 = -1,
    zero = 0,
    pos1 = 1,
    pos2 = 2,
};
static_assert(
        dplx::dp::encodable<signed_test_enum, dp_tests::test_output_stream<>>);
static_assert(
        dplx::dp::decodable<signed_test_enum, dp_tests::test_input_stream>);
static_assert(dplx::dp::tag_invocable<dplx::dp::encoded_size_of_fn,
                                      signed_test_enum const &>);

enum class unsigned_test_enum : unsigned
{
    zero = 0,
    pos1 = 1,
    pos2 = 2,
};
static_assert(dplx::dp::encodable<unsigned_test_enum,
                                  dp_tests::test_output_stream<>>);
static_assert(
        dplx::dp::decodable<unsigned_test_enum, dp_tests::test_input_stream>);
static_assert(dplx::dp::tag_invocable<dplx::dp::encoded_size_of_fn,
                                      unsigned_test_enum const &>);

enum unscoped_test_enum
{
};

static_assert(dplx::dp::encodable<unscoped_test_enum,
                                  dp_tests::test_output_stream<>>);
static_assert(
        dplx::dp::decodable<unscoped_test_enum, dp_tests::test_input_stream>);
static_assert(dplx::dp::tag_invocable<dplx::dp::encoded_size_of_fn,
                                      unscoped_test_enum const &>);

inline auto operator<<(std::ostream &stream, signed_test_enum value) noexcept
        -> std::ostream &
{
    auto const bits = dplx::dp::detail::to_underlying(value);
    if (bits < -2 || bits > 2)
    {
        return stream << "[invalid signed_test_enum value: " << bits << "]";
    }
    char const *strs[] = {"neg2", "neg1", "zero", "pos1", "pos2"};

    return stream << "signed_test_enum::" << strs[bits + 2];
}
inline auto operator<<(std::ostream &stream, unsigned_test_enum value) noexcept
        -> std::ostream &
{
    auto const bits = dplx::dp::detail::to_underlying(value);
    if (bits > 2u)
    {
        return stream << "[invalid unsigned_test_enum value: " << bits << "]";
    }
    char const *strs[] = {"zero", "pos1", "pos2"};

    return stream << "signed_test_enum::" << strs[bits];
}
inline auto operator<<(std::ostream &stream, unscoped_test_enum value) noexcept
        -> std::ostream &
{
    auto const bits = dplx::dp::detail::to_underlying(value);
    return stream << "[unscoped_test_enum value: " << bits << "]";
}

} // namespace

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(enum_codec)

static_assert(!dplx::dp::codable_enum<std::byte>);
static_assert(!dplx::dp::decodable<std::byte, test_input_stream>);
static_assert(!dplx::dp::encodable<std::byte, test_output_stream<>>);
static_assert(
        !dplx::dp::tag_invocable<dplx::dp::encoded_size_of_fn, std::byte>);

BOOST_FIXTURE_TEST_SUITE(encoder, default_encoding_fixture)

BOOST_AUTO_TEST_CASE(signed_neg2)
{
    auto const testValue = signed_test_enum::neg2;

    DPLX_TEST_RESULT(dplx::dp::encode(encodingBuffer, testValue));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0]
               == (to_byte(dplx::dp::type_code::negint) | std::byte{1}));
}
BOOST_AUTO_TEST_CASE(signed_neg1)
{
    auto testValue = signed_test_enum::neg1;

    DPLX_TEST_RESULT(dplx::dp::encode(encodingBuffer, testValue));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0]
               == (to_byte(dplx::dp::type_code::negint) | std::byte{0}));
}
BOOST_AUTO_TEST_CASE(signed_zero)
{
    auto const testValue = signed_test_enum::zero;

    DPLX_TEST_RESULT(dplx::dp::encode(encodingBuffer, testValue));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0]
               == (to_byte(dplx::dp::type_code::posint) | std::byte{0}));
}
BOOST_AUTO_TEST_CASE(signed_pos2)
{
    DPLX_TEST_RESULT(dplx::dp::encode(encodingBuffer, signed_test_enum::pos2));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0]
               == (to_byte(dplx::dp::type_code::posint) | std::byte{2}));
}

BOOST_AUTO_TEST_CASE(unsigned_zero)
{
    auto const testValue = unsigned_test_enum::zero;

    DPLX_TEST_RESULT(dplx::dp::encode(encodingBuffer, testValue));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0});
}
BOOST_AUTO_TEST_CASE(unsigned_pos2)
{
    DPLX_TEST_RESULT(
            dplx::dp::encode(encodingBuffer, unsigned_test_enum::pos2));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{2});
}

BOOST_AUTO_TEST_CASE(unscoped)
{
    DPLX_TEST_RESULT(dplx::dp::encode(encodingBuffer, unscoped_test_enum{}));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(decoder)

BOOST_AUTO_TEST_CASE(signed_neg2)
{
    auto const bytes = make_byte_array<32>({0x21});
    test_input_stream istream(bytes);

    signed_test_enum out;
    auto rx = dplx::dp::decode(istream, out);

    DPLX_TEST_RESULT(rx);
    BOOST_TEST(out == signed_test_enum::neg2);
}
BOOST_AUTO_TEST_CASE(signed_neg1)
{
    auto const bytes = make_byte_array<32>({0x20});
    test_input_stream istream(bytes);

    auto rx = dplx::dp::decode(dplx::dp::as_value<signed_test_enum>, istream);

    DPLX_TEST_RESULT(rx);
    BOOST_TEST(rx.assume_value() == signed_test_enum::neg1);
}
BOOST_AUTO_TEST_CASE(signed_zero)
{
    auto const bytes = make_byte_array<32>({0x00});
    test_input_stream istream(bytes);

    signed_test_enum out;
    auto rx = dplx::dp::decode(istream, out);

    DPLX_TEST_RESULT(rx);
    BOOST_TEST(out == signed_test_enum::zero);
}
BOOST_AUTO_TEST_CASE(signed_pos2)
{
    auto const bytes = make_byte_array<32>({0x02});
    test_input_stream istream(bytes);

    signed_test_enum out;
    auto rx = dplx::dp::decode(istream, out);

    DPLX_TEST_RESULT(rx);
    BOOST_TEST(out == signed_test_enum::pos2);
}

BOOST_AUTO_TEST_CASE(unsigned_zero)
{
    auto const bytes = make_byte_array<32>({0x00});
    test_input_stream istream(bytes);

    unsigned_test_enum out;
    auto rx = dplx::dp::decode(istream, out);

    DPLX_TEST_RESULT(rx);
    BOOST_TEST(out == unsigned_test_enum::zero);
}
BOOST_AUTO_TEST_CASE(unsigned_pos2)
{
    auto const bytes = make_byte_array<32>({0x02});
    test_input_stream istream(bytes);

    auto rx = dplx::dp::decode(dplx::dp::as_value<unsigned_test_enum>, istream);

    DPLX_TEST_RESULT(rx);
    BOOST_TEST(rx.assume_value() == unsigned_test_enum::pos2);
}

BOOST_AUTO_TEST_CASE(unscoped_zero)
{
    auto const bytes = make_byte_array<32>({0x00});
    test_input_stream istream(bytes);

    unscoped_test_enum out;
    auto rx = dplx::dp::decode(istream, out);

    DPLX_TEST_RESULT(rx);
    BOOST_TEST(out == unscoped_test_enum{});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
