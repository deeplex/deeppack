
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/decoder/object_utils.hpp>

#include <dplx/dp/decoder/core.hpp>
#include <dplx/dp/streams/memory_input_stream.hpp>

#include "boost-test.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(decoder)

BOOST_AUTO_TEST_SUITE(object_utils)

using dplx::dp::errc;
using dplx::dp::fixed_u8string;
using dplx::dp::named_property_def;
using dplx::dp::object_def;
using dplx::dp::property_def;

static_assert(std::same_as<dplx::dp::fixed_u8string<5>,
                           decltype(dplx::dp::fixed_u8string(u8"hello"))>);

static_assert(
    std::same_as<dplx::dp::fixed_u8string<16>,
                 typename std::common_type<dplx::dp::fixed_u8string<1>,
                                           dplx::dp::fixed_u8string<5>>::type>);

BOOST_AUTO_TEST_CASE(object_head_decode_unversioned)
{
    auto bytes = make_byte_array<32>({0xA3});
    test_input_stream istream{bytes};

    auto rx = dplx::dp::parse_object_head(istream, std::false_type{});
    DPLX_REQUIRE_RESULT(rx);
    auto const [numProps, version] = rx.assume_value();
    BOOST_TEST(numProps == 3);
    BOOST_TEST(version == dplx::dp::null_def_version);
}

BOOST_AUTO_TEST_CASE(object_head_decode_versioned_0)
{
    auto bytes = make_byte_array<32>({0xAB, 0, 0});
    test_input_stream istream{bytes};

    auto rx = dplx::dp::parse_object_head(istream, std::true_type{});
    DPLX_REQUIRE_RESULT(rx);
    auto const [numProps, version] = rx.assume_value();
    BOOST_TEST(numProps == 0x0B - 1);
    BOOST_TEST(version == 0);
}

BOOST_AUTO_TEST_CASE(object_head_decode_versioned_5)
{
    auto bytes = make_byte_array<32>({0xAB, 0, 5});
    test_input_stream istream{bytes};

    auto rx = dplx::dp::parse_object_head(istream, std::true_type{});
    DPLX_REQUIRE_RESULT(rx);
    auto const [numProps, version] = rx.assume_value();
    BOOST_TEST(numProps == 0x0B - 1);
    BOOST_TEST(version == 5);
}

struct test_tuple
{
    std::uint64_t ma;
    std::uint32_t mb;
    std::uint32_t mc;
};

constexpr object_def<property_def<1, &test_tuple::ma>{}> test_tuple_def_1{};
static_assert(!test_tuple_def_1.has_optional_properties);

constexpr object_def<property_def<1, &test_tuple::ma>{},
                     property_def<23, &test_tuple::mb>{}>
    test_tuple_def_2{};
static_assert(!test_tuple_def_2.has_optional_properties);

constexpr object_def<property_def<1, &test_tuple::ma>{},
                     property_def<23, &test_tuple::mb>{},
                     property_def<64, &test_tuple::mc>{}>
    test_tuple_def_3{};
static_assert(!test_tuple_def_3.has_optional_properties);

constexpr object_def<property_def<1, &test_tuple::ma>{},
                     property_def<23, &test_tuple::mb>{false},
                     property_def<64, &test_tuple::mc>{}>
    test_tuple_def_3_with_optional{};
static_assert(test_tuple_def_3_with_optional.has_optional_properties);

BOOST_AUTO_TEST_CASE(prop_decode_1)
{
    auto bytes = make_byte_array<32>({1, 23});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dplx::dp::decode_object_property<test_tuple_def_3>(istream, out);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 23u);
    BOOST_TEST(out.mb == 0);
    BOOST_TEST(out.mc == 0);
}

BOOST_AUTO_TEST_CASE(prop_decode_2)
{
    auto bytes = make_byte_array<32>({23, 0x18, 0xEF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dplx::dp::decode_object_property<test_tuple_def_3>(istream, out);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0);
    BOOST_TEST(out.mb == 0xEF);
    BOOST_TEST(out.mc == 0);
}

BOOST_AUTO_TEST_CASE(prop_decode_3)
{
    auto bytes = make_byte_array<32>({0x18, 64, 0x1A, 0xDE, 0xAD, 0xBE, 0xEF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dplx::dp::decode_object_property<test_tuple_def_3>(istream, out);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0);
    BOOST_TEST(out.mb == 0);
    BOOST_TEST(out.mc == 0xDEADBEEF);
}

BOOST_AUTO_TEST_CASE(prop_decode_reject_unknown_prop)
{
    auto bytes = make_byte_array<32>({0x18, 65, 0x11});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dplx::dp::decode_object_property<test_tuple_def_3>(istream, out);
    BOOST_TEST(rx.error() == errc::unknown_property);
}

BOOST_AUTO_TEST_CASE(prop_decode_reject_invalid_id_type)
{
    auto bytes = make_byte_array<32>({0x20, 0x11});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dplx::dp::decode_object_property<test_tuple_def_3>(istream, out);
    BOOST_TEST(rx.error() == errc::item_type_mismatch);
}

BOOST_AUTO_TEST_CASE(def_1)
{
    auto bytes = make_byte_array<32>({1, 0x17});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx =
        dplx::dp::decode_object_properties<test_tuple_def_1>(istream, out, 1);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0x17);
    BOOST_TEST(out.mb == 0);
    BOOST_TEST(out.mc == 0);
}

BOOST_AUTO_TEST_CASE(def_2)
{
    auto bytes = make_byte_array<32>({23, 0x18, 0xEF, 1, 0x19, 0x48, 0xCF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx =
        dplx::dp::decode_object_properties<test_tuple_def_2>(istream, out, 2);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0x48CF);
    BOOST_TEST(out.mb == 0xEF);
    BOOST_TEST(out.mc == 0);
}

BOOST_AUTO_TEST_CASE(def_3)
{
    auto bytes = make_byte_array<32>({23,
                                      0x18,
                                      0xEF,
                                      1,
                                      0x19,
                                      0x48,
                                      0xCF,
                                      0x18,
                                      64,
                                      0x1A,
                                      0xDE,
                                      0xAD,
                                      0xBE,
                                      0xEF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx =
        dplx::dp::decode_object_properties<test_tuple_def_3>(istream, out, 3);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0x48CF);
    BOOST_TEST(out.mb == 0xEF);
    BOOST_TEST(out.mc == 0xDEADBEEF);
}

BOOST_AUTO_TEST_CASE(def_3_reject_missing_property)
{
    auto bytes = make_byte_array<32>({23, 0x18, 0xEF, 1, 0x19, 0x48, 0xCF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx =
        dplx::dp::decode_object_properties<test_tuple_def_3>(istream, out, 2);
    BOOST_TEST(rx.error() == errc::required_object_property_missing);
}

BOOST_AUTO_TEST_CASE(def_3_with_optional_set)
{
    auto bytes = make_byte_array<32>({23,
                                      0x18,
                                      0xEF,
                                      1,
                                      0x19,
                                      0x48,
                                      0xCF,
                                      0x18,
                                      64,
                                      0x1A,
                                      0xDE,
                                      0xAD,
                                      0xBE,
                                      0xEF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx =
        dplx::dp::decode_object_properties<test_tuple_def_3_with_optional>(
            istream, out, 3);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0x48CF);
    BOOST_TEST(out.mb == 0xEF);
    BOOST_TEST(out.mc == 0xDEADBEEF);
}

BOOST_AUTO_TEST_CASE(def_3_with_optional_not_set)
{
    auto bytes = make_byte_array<32>(
        {0x18, 64, 0x1A, 0xDE, 0xAD, 0xBE, 0xEF, 1, 0x19, 0x48, 0xCF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx =
        dplx::dp::decode_object_properties<test_tuple_def_3_with_optional>(
            istream, out, 2);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0x48CF);
    BOOST_TEST(out.mb == 0);
    BOOST_TEST(out.mc == 0xDEADBEEF);
}

BOOST_AUTO_TEST_CASE(def_3_with_optional_reject_missing_property)
{
    auto bytes = make_byte_array<32>({23, 0x18, 0xEF, 1, 0x19, 0x48, 0xCF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx =
        dplx::dp::decode_object_properties<test_tuple_def_3_with_optional>(
            istream, out, 2);
    BOOST_TEST(rx.error() == errc::required_object_property_missing);
}

class custom_with_layout_descriptor
{
    std::uint64_t ma;
    std::uint32_t mb;
    std::uint32_t mc;
    std::uint32_t md;

public:
    constexpr custom_with_layout_descriptor() noexcept = default;

    static constexpr object_def<
        property_def<1, &custom_with_layout_descriptor::ma>{},
        property_def<2, &custom_with_layout_descriptor::mb>{},
        property_def<26, &custom_with_layout_descriptor::mc>{},
        property_def<36, &custom_with_layout_descriptor::md>{}>
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
static_assert(dplx::dp::packable<custom_with_layout_descriptor>);
static_assert(dplx::dp::packable_object<custom_with_layout_descriptor>);

BOOST_AUTO_TEST_CASE(custom_with_layout_descriptor_decoding)
{
    using test_encoder = dplx::dp::basic_decoder<custom_with_layout_descriptor,
                                                 test_input_stream>;

    test_encoder subject{};

    auto bytes = make_byte_array<32>(
        {0b101'00000 | 4, 2, 7, 1, 0x13, 24, 26, 4, 24, 36, 0x14});
    test_input_stream istream{bytes};

    custom_with_layout_descriptor t{};
    auto rx = subject(istream, t);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(t.a() == 0x13);
    BOOST_TEST(t.b() == 0x07);
    BOOST_TEST(t.c() == 0x04);
    BOOST_TEST(t.d() == 0x14);
}

// using namespace dplx::dp::string_literals;

class custom_with_named_layout_descriptor
{
    std::uint64_t ma;
    std::uint32_t mb;
    std::uint32_t mc;
    std::uint32_t md;

public:
    constexpr custom_with_named_layout_descriptor() noexcept = default;

    static constexpr named_property_def<
        u8"a",
        &custom_with_named_layout_descriptor::ma>
        pma{};

    static constexpr object_def<
        named_property_def<u8"b", &custom_with_named_layout_descriptor::mb>{},
        named_property_def<u8"c", &custom_with_named_layout_descriptor::mc>{},
        named_property_def<u8"d", &custom_with_named_layout_descriptor::md>{},
        named_property_def<u8"nonce",
                           &custom_with_named_layout_descriptor::ma>{}>
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
static_assert(dplx::dp::packable<custom_with_named_layout_descriptor>);

//
// if this test fails on GCC, but not on MSVC: please check whether the
// WORKAROUND version range specified in <dplx/dp/object_def.hpp> applies
// to your GCC version
//
BOOST_AUTO_TEST_CASE(custom_with_named_layout_descriptor_decoding)
{
    using test_encoder =
        dplx::dp::basic_decoder<custom_with_named_layout_descriptor,
                                test_input_stream>;

    test_encoder subject{};

    auto bytes = make_byte_array<32, int>({0b101'00000 | 4,
                                           0x61,
                                           'b',
                                           7,
                                           0x65,
                                           'n',
                                           'o',
                                           'n',
                                           'c',
                                           'e',
                                           0x13,
                                           0x61,
                                           'c',
                                           0x04,
                                           0x61,
                                           'd',
                                           0x14});
    test_input_stream istream{bytes};

    custom_with_named_layout_descriptor t{};
    auto rx = subject(istream, t);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(t.a() == 0x13);
    BOOST_TEST(t.b() == 0x07);
    BOOST_TEST(t.c() == 0x04);
    BOOST_TEST(t.d() == 0x14);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
