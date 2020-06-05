
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/type_encoder.hpp>

#include <boost/mp11.hpp>

#include "boost-test.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(type_encoder)

struct integer_encoding_fixture
{
    test_output_stream<> encodingBuffer{};
};

BOOST_FIXTURE_TEST_SUITE(integer, integer_encoding_fixture)

using boost::mp11::mp_append;
using boost::mp11::mp_copy_if;
using boost::mp11::mp_list;
using boost::mp11::mp_transform;

using mp_int_type_list = mp_list<signed char, short, int, long, long long>;

template <typename T, std::size_t l>
using mp_type_min_size = std::bool_constant<(l < sizeof(T))>;
template <typename T>
using mp_type_min_1B = mp_type_min_size<T, 0>;
template <typename T>
using mp_type_min_2B = mp_type_min_size<T, 1>;
template <typename T>
using mp_type_min_4B = mp_type_min_size<T, 3>;
template <typename T>
using mp_type_min_8B = mp_type_min_size<T, 7>;

using mp_s1B_test_types = mp_copy_if<mp_int_type_list, mp_type_min_1B>;
using mp_s2B_test_types = mp_copy_if<mp_int_type_list, mp_type_min_2B>;
using mp_s4B_test_types = mp_copy_if<mp_int_type_list, mp_type_min_4B>;
using mp_s8B_test_types = mp_copy_if<mp_int_type_list, mp_type_min_8B>;

using mp_u1B_test_types =
    mp_append<mp_s2B_test_types,
              mp_transform<std::make_unsigned_t, mp_s1B_test_types>>;
using mp_u2B_test_types =
    mp_append<mp_s4B_test_types,
              mp_transform<std::make_unsigned_t, mp_s2B_test_types>>;
using mp_u4B_test_types =
    mp_append<mp_s8B_test_types,
              mp_transform<std::make_unsigned_t, mp_s4B_test_types>>;
using mp_u8B_test_types = mp_transform<std::make_unsigned_t, mp_s8B_test_types>;

enum class type_prefix_byte : std::uint8_t
{
    pint_cat = 0b000'00000,
    pint08 = pint_cat | 24 | 0,
    pint16,
    pint32,
    pint64,

    nint_cat = 0b001'00000,
    nint08 = nint_cat | 24 | 0,
    nint16,
    nint32,
    nint64,
};

constexpr auto operator==(type_prefix_byte lhs, std::byte rhs) -> bool
{
    return static_cast<std::uint8_t>(lhs) == static_cast<std::uint8_t>(rhs);
}

inline auto boost_test_print_type(std::ostream &s, type_prefix_byte c)
    -> std::ostream &
{
    fmt::print(s, FMT_STRING("{:2x}"), static_cast<std::uint8_t>(c));
    return s;
}

using test_encoder = dplx::dp::type_encoder<test_output_stream<>>;

#pragma region Appendix A.Examples

BOOST_AUTO_TEST_CASE_TEMPLATE(example_1_0x00, T, mp_u1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x00));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0x00});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(example_2_0x01, T, mp_u1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x01));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0x01});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(example_3_0x0a, T, mp_u1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x0a));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0x0a});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(example_6_0x19, T, mp_u1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x19));

    BOOST_TEST(encodingBuffer.size() == 2u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint08);

    auto const encoded = make_byte_array(0x19);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(example_7_0x64, T, mp_u1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x64));

    BOOST_TEST(encodingBuffer.size() == 2u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint08);

    auto const encoded = make_byte_array(0x64);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(example_8_0x03e8, T, mp_u2B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x03e8));

    BOOST_TEST(encodingBuffer.size() == 3u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint16);

    auto const encoded = make_byte_array(0x03, 0xe8);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(example_9_0x000f_4240, T, mp_u4B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x000f'4240));

    BOOST_TEST(encodingBuffer.size() == 5u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint32);

    auto const encoded = make_byte_array(0x00, 0x0f, 0x42, 0x40);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(example_10_0x0000_00e8_d4a5_1000,
                              T,
                              mp_u8B_test_types)
{
    DPLX_TEST_RESULT(
        test_encoder::integer<T>(encodingBuffer, 0x0000'00e8'd4a5'1000));

    BOOST_TEST(encodingBuffer.size() == 9u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint64);

    auto const encoded =
        make_byte_array(0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(example_16, T, mp_s1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, -10));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0x29});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(example_17, T, mp_s1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, -100));

    BOOST_TEST(encodingBuffer.size() == 2u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::nint08);

    auto const encoded = make_byte_array(0x63);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(example_18, T, mp_s2B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, -1000));

    BOOST_TEST(encodingBuffer.size() == 3u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::nint16);

    auto const encoded = make_byte_array(0x03, 0xe7);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

#pragma endregion

#pragma region signed

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_8B_negative_lower_bound,
                              T,
                              mp_s8B_test_types)
{
    DPLX_TEST_RESULT(
        test_encoder::integer<T>(encodingBuffer, -1 - 0x7fff'ffff'ffff'ffffll));

    BOOST_TEST(encodingBuffer.size() == 9u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::nint64);

    auto const encoded =
        make_byte_array(0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_8B_negative_upper_bound,
                              T,
                              mp_s8B_test_types)
{
    DPLX_TEST_RESULT(
        test_encoder::integer<T>(encodingBuffer, -1 - 0x1'0000'0000ll));

    BOOST_TEST(encodingBuffer.size() == 9u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::nint64);

    auto const encoded =
        make_byte_array(0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_4B_negative_lower_bound,
                              T,
                              mp_s4B_test_types)
{
    DPLX_TEST_RESULT(
        test_encoder::integer<T>(encodingBuffer, -1 - 0x7fff'ffffll));

    BOOST_TEST(encodingBuffer.size() == 5u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::nint32);

    auto const encoded = make_byte_array(0x7f, 0xff, 0xff, 0xff);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_4B_negative_upper_bound,
                              T,
                              mp_s4B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, -1 - 0x1'0000));

    BOOST_TEST(encodingBuffer.size() == 5u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::nint32);

    auto const encoded = make_byte_array(0x00, 0x01, 0x00, 0x00);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_2B_negative_lower_bound,
                              T,
                              mp_s2B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, -1 - 0x7fff));

    BOOST_TEST(encodingBuffer.size() == 3u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::nint16);

    auto const encoded = make_byte_array(0x7f, 0xff);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_2B_negative_upper_bound,
                              T,
                              mp_s2B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, -1 - 0x100));

    BOOST_TEST(encodingBuffer.size() == 3u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::nint16);

    auto const encoded = make_byte_array(0x01, 0x00);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_1B_negative_lower_bound,
                              T,
                              mp_s1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, -1 - 0x7f));

    BOOST_TEST(encodingBuffer.size() == 2u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::nint08);

    auto const encoded = make_byte_array(0x7f);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_1B_negative_upper_bound,
                              T,
                              mp_s1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, -1 - 0x18));

    BOOST_TEST(encodingBuffer.size() == 2u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::nint08);
    BOOST_TEST(encodingBuffer.data()[1] == std::byte{0x18});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_inline_lower_bound, T, mp_s1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, -1 - 0x17));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0b001'10111});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_inline_negative_one, T, mp_s1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, -0x01));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0b001'00000});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_inline_zero, T, mp_s1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x00));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0b0000'0000});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_inline_upper_bound, T, mp_s1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x17));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0b000'10111});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_1B_positive_lower_bound,
                              T,
                              mp_s1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x18));

    BOOST_TEST(encodingBuffer.size() == 2u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint08);

    auto const encoded = make_byte_array(0x18);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_1B_positive_upper_bound,
                              T,
                              mp_s1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x7f));

    BOOST_TEST(encodingBuffer.size() == 2u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint08);

    auto const encoded = make_byte_array(0x7f);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_2B_positive_lower_bound,
                              T,
                              mp_s2B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x100));

    BOOST_TEST(encodingBuffer.size() == 3u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint16);

    auto const encoded = make_byte_array(0x01, 0x00);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_2B_positive_upper_bound,
                              T,
                              mp_s2B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x7fff));

    BOOST_TEST(encodingBuffer.size() == 3u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint16);

    auto const encoded = make_byte_array(0x7f, 0xff);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_4B_positive_lower_bound,
                              T,
                              mp_s4B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x1'0000));

    BOOST_TEST(encodingBuffer.size() == 5u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint32);

    auto const encoded = make_byte_array(0x00, 0x01, 0x00, 0x00);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_4B_positive_upper_bound,
                              T,
                              mp_s4B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x7fff'ffff));

    BOOST_TEST(encodingBuffer.size() == 5u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint32);

    auto const encoded = make_byte_array(0x7f, 0xff, 0xff, 0xff);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_8B_positive_lower_bound,
                              T,
                              mp_s8B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x1'0000'0000));

    BOOST_TEST(encodingBuffer.size() == 9u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint64);

    auto const encoded =
        make_byte_array(0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(signed_8B_positive_upper_bound,
                              T,
                              mp_s8B_test_types)
{
    DPLX_TEST_RESULT(
        test_encoder::integer<T>(encodingBuffer, 0x7fff'ffff'ffff'ffff));

    BOOST_TEST(encodingBuffer.size() == 9u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint64);

    auto const encoded =
        make_byte_array(0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
    BOOST_TEST(byte_span(encodingBuffer).subspan(1) == encoded,
               boost::test_tools::per_element{});
}

#pragma endregion

#pragma region unsigned

BOOST_AUTO_TEST_CASE_TEMPLATE(unsigned_inline_upper_bound, T, mp_u1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x17));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == std::byte{0x17});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unsigned_1B_lower_bound, T, mp_u1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x18));

    BOOST_TEST(encodingBuffer.size() == 2u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint08);
    BOOST_TEST(encodingBuffer.data()[1] == std::byte{0x18});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unsigned_1B_upper_bound, T, mp_u1B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0xffu));

    BOOST_TEST(encodingBuffer.size() == 2u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint08);
    BOOST_TEST(encodingBuffer.data()[1] == std::byte{0xff});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unsigned_2B_lower_bound, T, mp_u2B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x0100));

    BOOST_TEST(encodingBuffer.size() == 3u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint16);
    BOOST_TEST(encodingBuffer.data()[1] == std::byte{0x01});
    BOOST_TEST(encodingBuffer.data()[2] == std::byte{0x00});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unsigned_2B_upper_bound, T, mp_u2B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0xffff));

    BOOST_TEST(encodingBuffer.size() == 3u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint16);
    BOOST_TEST(encodingBuffer.data()[1] == std::byte{0xff});
    BOOST_TEST(encodingBuffer.data()[2] == std::byte{0xff});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unsigned_4B_lower_bound, T, mp_u4B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x1'0000));

    BOOST_TEST(encodingBuffer.size() == 5u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint32);

    BOOST_TEST(encodingBuffer.data()[1] == std::byte{0x00});
    BOOST_TEST(encodingBuffer.data()[2] == std::byte{0x01});
    BOOST_TEST(encodingBuffer.data()[3] == std::byte{0x00});
    BOOST_TEST(encodingBuffer.data()[4] == std::byte{0x00});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unsigned_4B_upper_bound, T, mp_u4B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0xffff'ffff));

    BOOST_TEST(encodingBuffer.size() == 5u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint32);

    BOOST_TEST(encodingBuffer.data()[1] == std::byte{0xff});
    BOOST_TEST(encodingBuffer.data()[2] == std::byte{0xff});
    BOOST_TEST(encodingBuffer.data()[3] == std::byte{0xff});
    BOOST_TEST(encodingBuffer.data()[4] == std::byte{0xff});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unsigned_8B_lower_bound, T, mp_u8B_test_types)
{
    DPLX_TEST_RESULT(test_encoder::integer<T>(encodingBuffer, 0x1'0000'0000));

    BOOST_TEST(encodingBuffer.size() == 9u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint64);

    BOOST_TEST(encodingBuffer.data()[1] == std::byte{0x00});
    BOOST_TEST(encodingBuffer.data()[2] == std::byte{0x00});
    BOOST_TEST(encodingBuffer.data()[3] == std::byte{0x00});
    BOOST_TEST(encodingBuffer.data()[4] == std::byte{0x01});

    BOOST_TEST(encodingBuffer.data()[5] == std::byte{0x00});
    BOOST_TEST(encodingBuffer.data()[6] == std::byte{0x00});
    BOOST_TEST(encodingBuffer.data()[7] == std::byte{0x00});
    BOOST_TEST(encodingBuffer.data()[8] == std::byte{0x00});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unsigned_8B_upper_bound, T, mp_u8B_test_types)
{
    DPLX_TEST_RESULT(
        test_encoder::integer<T>(encodingBuffer, 0xffff'ffff'ffff'ffff));

    BOOST_TEST(encodingBuffer.size() == 9u);
    BOOST_TEST(encodingBuffer.data()[0] == type_prefix_byte::pint64);

    BOOST_TEST(encodingBuffer.data()[1] == std::byte{0xff});
    BOOST_TEST(encodingBuffer.data()[2] == std::byte{0xff});
    BOOST_TEST(encodingBuffer.data()[3] == std::byte{0xff});
    BOOST_TEST(encodingBuffer.data()[4] == std::byte{0xff});

    BOOST_TEST(encodingBuffer.data()[5] == std::byte{0xff});
    BOOST_TEST(encodingBuffer.data()[6] == std::byte{0xff});
    BOOST_TEST(encodingBuffer.data()[7] == std::byte{0xff});
    BOOST_TEST(encodingBuffer.data()[8] == std::byte{0xff});
}

#pragma endregion

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
