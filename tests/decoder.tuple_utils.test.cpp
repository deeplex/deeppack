
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/decoder/tuple_utils.hpp>

#include <dplx/dp/decoder/core.hpp>

#include "boost-test.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(decoder)

BOOST_AUTO_TEST_SUITE(tuple_utils)

using dp::tuple_def;
using dp::tuple_member_def;
using dp::tuple_member_fun;

BOOST_AUTO_TEST_CASE(tuple_head_decode_unversioned)
{
    auto bytes = make_byte_array<32>({0x87});
    test_input_stream istream{bytes};

    auto rx = dp::parse_tuple_head(istream, std::false_type{});
    DPLX_REQUIRE_RESULT(rx);

    auto const [numProps, version] = rx.assume_value();
    BOOST_TEST(numProps == 7);
    BOOST_TEST(version == dp::null_def_version);
}

BOOST_AUTO_TEST_CASE(tuple_head_decode_versioned_00)
{
    auto bytes = make_byte_array<32>({0x8B, 0x00});
    test_input_stream istream{bytes};

    auto rx = dp::parse_tuple_head(istream, std::true_type{});
    DPLX_REQUIRE_RESULT(rx);

    auto const [numProps, version] = rx.assume_value();
    BOOST_TEST(numProps == 0x0B - 1);
    BOOST_TEST(version == 0x00u);
}

BOOST_AUTO_TEST_CASE(tuple_head_decode_versioned_ff)
{
    auto bytes = make_byte_array<32>({0x8B, 0x18, 0xFF});
    test_input_stream istream{bytes};

    auto rx = dp::parse_tuple_head(istream, std::true_type{});
    DPLX_REQUIRE_RESULT(rx);

    auto const [numProps, version] = rx.assume_value();
    BOOST_TEST(numProps == 0x0B - 1);
    BOOST_TEST(version == 0xFFu);
}

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

BOOST_AUTO_TEST_CASE(decode_def_1)
{
    auto bytes = make_byte_array<32>({0x1A, 0xDE, 0xAD, 0xBE, 0xEF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dp::decode_tuple_properties<test_tuple_def_1>(istream, out, 1);

    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0xDEADBEEFu);
    BOOST_TEST(out.mb == 0u);
    BOOST_TEST(out.mc == 0u);
}
BOOST_AUTO_TEST_CASE(decode_def_2)
{
    auto bytes = make_byte_array<32>({0x17, 0x1A, 0xDE, 0xAD, 0xBE, 0xEF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dp::decode_tuple_properties<test_tuple_def_2>(istream, out, 2);

    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0x17u);
    BOOST_TEST(out.mb == 0xDEADBEEFu);
    BOOST_TEST(out.mc == 0u);
}
BOOST_AUTO_TEST_CASE(decode_def_3)
{
    auto bytes = make_byte_array<32>(
            {0x1A, 0xDE, 0xAD, 0xBE, 0xEF, 0x07, 0x19, 0xFE, 0xFE});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dp::decode_tuple_properties<test_tuple_def_3>(istream, out, 3);

    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0xDEADBEEFu);
    BOOST_TEST(out.mb == 0x07u);
    BOOST_TEST(out.mc == 0xFEFEu);
}

BOOST_AUTO_TEST_CASE(def_3_reject_missing_member)
{
    auto bytes = make_byte_array<32>({0x17, 0x1A, 0xDE, 0xAD, 0xBE, 0xEF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dp::decode_tuple_properties<test_tuple_def_3>(istream, out, 2);
    BOOST_TEST_REQUIRE(rx.has_error());
    BOOST_TEST(rx.assume_error() == dp::errc::tuple_size_mismatch);
}

BOOST_AUTO_TEST_CASE(def_1_reject_too_many_members)
{
    auto bytes = make_byte_array<32>({0x17, 0x1A, 0xDE, 0xAD, 0xBE, 0xEF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dp::decode_tuple_properties<test_tuple_def_1>(istream, out, 2);
    BOOST_TEST_REQUIRE(rx.has_error());
    BOOST_TEST(rx.assume_error() == dp::errc::tuple_size_mismatch);
}

class custom_with_layout_descriptor
{
    std::uint64_t ma;
    std::uint32_t mb;
    std::uint32_t mc;
    std::uint32_t md;

    struct mc_accessor
        : dp::member_accessor_base<custom_with_layout_descriptor, std::uint32_t>
    {
        auto operator()(auto &self) const noexcept
        {
            return &self.mc;
        }
    };

public:
    constexpr custom_with_layout_descriptor() noexcept = default;

    static constexpr tuple_def<
            tuple_member_def<&custom_with_layout_descriptor::mb>{},
            tuple_member_def<&custom_with_layout_descriptor::ma>{},
            tuple_member_fun<mc_accessor>{},
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
static_assert(dp::packable_tuple<custom_with_layout_descriptor>);

BOOST_AUTO_TEST_CASE(custom_with_layout_descriptor_decoding)
{
    auto bytes = make_byte_array<32>({0b100'00100, 0x13, 0x09, 0x15, 0x17});
    test_input_stream istream{bytes};

    using test_decoder = dp::basic_decoder<custom_with_layout_descriptor,
                                           test_input_stream>;
    test_decoder subject{};
    custom_with_layout_descriptor t{};
    auto rx = subject(istream, t);

    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(t.a() == 0x09);
    BOOST_TEST(t.b() == 0x13);
    BOOST_TEST(t.c() == 0x15);
    BOOST_TEST(t.d() == 0x17);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
