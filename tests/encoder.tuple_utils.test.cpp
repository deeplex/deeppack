
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/encoder/tuple_utils.hpp>

#include <dplx/dp/encoder/core.hpp>

#include "boost-test.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(encoder)

BOOST_AUTO_TEST_SUITE(tuple_utils)

using dplx::dp::tuple_def;
using dplx::dp::tuple_member_def;

struct test_tuple
{
    std::uint32_t ma;
    std::uint64_t mb;
    std::uint32_t mc;
};

constexpr tuple_def<tuple_member_def<&test_tuple::ma>{}> test_tuple_def_1{};

constexpr tuple_def<tuple_member_def<&test_tuple::ma>{},
                    tuple_member_def<&test_tuple::mb>{}>
        test_tuple_def_2{};

constexpr tuple_def<tuple_member_def<&test_tuple::ma>{},
                    tuple_member_def<&test_tuple::mb>{},
                    tuple_member_def<&test_tuple::mc>{}>
        test_tuple_def_3{};

BOOST_AUTO_TEST_CASE(encode_def_1)
{
    auto bytes = make_byte_array<6>({0x81, 0x1A, 0xDE, 0xAD, 0xBE, 0xEF});
    test_tuple const t{0xDEADBEEFu, 0x07u, 0xFEFEu};

    test_output_stream<> ostream{};
    auto rx = dplx::dp::encode_tuple<test_tuple_def_1>(ostream, t);

    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(byte_span(bytes) == byte_span(ostream),
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(encode_def_2)
{
    auto bytes = make_byte_array<7>({0x82, 0x1A, 0xDE, 0xAD, 0xBE, 0xEF, 0x07});
    test_tuple const t{0xDEADBEEFu, 0x07u, 0xFEFEu};

    test_output_stream<> ostream{};
    auto rx = dplx::dp::encode_tuple<test_tuple_def_2>(ostream, t);

    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(byte_span(bytes) == byte_span(ostream),
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(encode_def_2_with_version)
{
    auto bytes = make_byte_array<8>(
            {0x83, 0x0D, 0x1A, 0xDE, 0xAD, 0xBE, 0xEF, 0x07});
    test_tuple const t{0xDEADBEEFu, 0x07u, 0xFEFEu};

    test_output_stream<> ostream{};
    auto rx = dplx::dp::encode_tuple<test_tuple_def_2>(ostream, t, 0x0D);

    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(byte_span(bytes) == byte_span(ostream),
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(encode_def_3_with_version)
{
    auto bytes = make_byte_array<10>(
            {0x83, 0x1A, 0xDE, 0xAD, 0xBE, 0xEF, 0x07, 0x19, 0xFE, 0xFE});
    test_tuple const t{0xDEADBEEFu, 0x07u, 0xFEFEu};

    test_output_stream<> ostream{};
    auto rx = dplx::dp::encode_tuple<test_tuple_def_3>(ostream, t);

    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(byte_span(bytes) == byte_span(ostream),
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(encode_def_3)
{
    auto bytes = make_byte_array<12>({0x84, 0x18, 0xFF, 0x1A, 0xDE, 0xAD, 0xBE,
                                      0xEF, 0x07, 0x19, 0xFE, 0xFE});
    test_tuple const t{0xDEADBEEFu, 0x07u, 0xFEFEu};

    test_output_stream<> ostream{};
    auto rx = dplx::dp::encode_tuple<test_tuple_def_3>(ostream, t, 0xFF);

    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(byte_span(bytes) == byte_span(ostream),
               boost::test_tools::per_element{});
}

class custom_with_layout_descriptor
{
    std::uint64_t ma;
    std::uint32_t mb;
    std::uint32_t mc;
    std::uint32_t md;

public:
    constexpr custom_with_layout_descriptor() noexcept = default;
    constexpr custom_with_layout_descriptor(std::uint64_t a,
                                            std::uint32_t b,
                                            std::uint32_t c,
                                            std::uint32_t d)
        : ma(a)
        , mb(b)
        , mc(c)
        , md(d)
    {
    }

    static constexpr tuple_def<
            tuple_member_def<&custom_with_layout_descriptor::mb>{},
            tuple_member_def<&custom_with_layout_descriptor::ma>{},
            tuple_member_def<&custom_with_layout_descriptor::mc>{},
            tuple_member_def<&custom_with_layout_descriptor::md>{}>
            layout_descriptor{};

    auto a() const noexcept -> std::uint64_t
    {
        return ma;
    }
    auto b() const noexcept -> std::uint32_t
    {
        return mb;
    }
    auto c() const noexcept -> std::uint32_t
    {
        return mc;
    }
    auto d() const noexcept -> std::uint32_t
    {
        return md;
    }
};
static_assert(dplx::dp::packable_tuple<custom_with_layout_descriptor>);

BOOST_AUTO_TEST_CASE(custom_with_layout_descriptor_encoding)
{
    auto bytes = make_byte_array<5>({0b100'00100, 0x13, 0x09, 0x15, 0x17});
    custom_with_layout_descriptor const t{0x09, 0x13, 0x15, 0x17};

    using test_encoder = dplx::dp::basic_encoder<custom_with_layout_descriptor,
                                                 test_output_stream<>>;
    test_output_stream<> ostream{};
    test_encoder subject{};
    auto rx = subject(ostream, t);

    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(byte_span(bytes) == byte_span(ostream),
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
