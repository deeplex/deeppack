
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/encoder/object_utils.hpp>

#include <dplx/dp/encoder/core.hpp>

#include "boost-test.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(encoder)

BOOST_AUTO_TEST_SUITE(object_utils)

using dp::fixed_u8string;
using dp::named_property_def;
using dp::object_def;
using dp::property_def;
using dp::property_fun;

struct test_object
{
    std::uint64_t ma;
    std::uint32_t mb;
    std::uint32_t mc;
};

constexpr object_def<property_def<1, &test_object::ma>{}> test_object_def_1{};
static_assert(!test_object_def_1.has_optional_properties);

constexpr object_def<property_def<1, &test_object::ma>{},
                     property_def<23, &test_object::mb>{}>
        test_object_def_2{};
static_assert(!test_object_def_2.has_optional_properties);

constexpr object_def<property_def<1, &test_object::ma>{},
                     property_def<23, &test_object::mb>{},
                     property_def<64, &test_object::mc>{}>
        test_object_def_3{};
static_assert(!test_object_def_3.has_optional_properties);

constexpr object_def<property_def<1, &test_object::ma>{},
                     property_def<23, &test_object::mb>{false},
                     property_def<64, &test_object::mc>{}>
        test_object_def_3_with_optional{};
static_assert(test_object_def_3_with_optional.has_optional_properties);

class custom_with_layout_descriptor
{
    std::uint64_t ma;
    std::uint32_t mb;
    std::uint32_t mc;
    std::uint32_t md;

    test_object msub;

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
    constexpr custom_with_layout_descriptor(std::uint64_t a,
                                            std::uint32_t b,
                                            std::uint32_t c,
                                            std::uint32_t d,
                                            test_object sub) noexcept
        : ma(a)
        , mb(b)
        , mc(c)
        , md(d)
        , msub(sub)
    {
    }

    static constexpr object_def<
            property_def<1, &custom_with_layout_descriptor::ma>{},
            property_def<2, &custom_with_layout_descriptor::mb>{},
            property_def<5,
                         &custom_with_layout_descriptor::msub,
                         &test_object::mc>{},
            property_fun<26, mc_accessor>{},
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
static_assert(dp::packable_object<custom_with_layout_descriptor>);
static_assert(
        dp::encodable<custom_with_layout_descriptor, test_output_stream<>>);

BOOST_AUTO_TEST_CASE(custom_with_layout_descriptor_encoding)
{
    using test_encoder = dp::basic_encoder<custom_with_layout_descriptor,
                                           test_output_stream<>>;

    auto bytes = make_byte_array<14>({0b101'00000 | 5, 1, 0x13, 2, 7, 5, 0x18,
                                      0x2a, 0x18, 26, 4, 0x18, 36, 0x14});

    test_encoder subject{};

    test_output_stream ostream{};

    custom_with_layout_descriptor const t{0x13, 0x07, 0x04, 0x14,
                                          test_object{0, 0, 0x2a}};
    auto rx = subject(ostream, t);
    DPLX_REQUIRE_RESULT(rx);

    BOOST_TEST(byte_span(bytes) == byte_span(ostream),
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(custom_with_layout_descriptor_size_of)
{
    custom_with_layout_descriptor const t{0x13, 0x07, 0x04, 0x14,
                                          test_object{0, 0, 0x2a}};

    auto const sizeOfT = dp::encoded_size_of(t);
    BOOST_TEST(sizeOfT == 14u);
}

class custom_with_named_layout_descriptor
{
    std::uint64_t ma;
    std::uint32_t mb;
    std::uint32_t mc;
    std::uint32_t md;

public:
    constexpr custom_with_named_layout_descriptor() noexcept = default;
    constexpr custom_with_named_layout_descriptor(std::uint64_t a,
                                                  std::uint32_t b,
                                                  std::uint32_t c,
                                                  std::uint32_t d) noexcept
        : ma(a)
        , mb(b)
        , mc(c)
        , md(d)
    {
    }

    static constexpr named_property_def<
            u8"a",
            &custom_with_named_layout_descriptor::ma>
            pma{};

    static constexpr object_def<
            named_property_def<u8"b",
                               &custom_with_named_layout_descriptor::mb>{},
            named_property_def<u8"c",
                               &custom_with_named_layout_descriptor::mc>{},
            named_property_def<u8"d",
                               &custom_with_named_layout_descriptor::md>{},
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
static_assert(dp::packable_object<custom_with_named_layout_descriptor>);
static_assert(dp::encodable<custom_with_named_layout_descriptor,
                            test_output_stream<>>);

BOOST_AUTO_TEST_CASE(custom_with_named_layout_descriptor_encoding)
{
    using test_encoder = dp::basic_encoder<custom_with_named_layout_descriptor,
                                           test_output_stream<>>;
    auto bytes = make_byte_array<17, int>({0b101'00000 | 4, 0x61, 'b', 7, 0x61,
                                           'c', 0x04, 0x61, 'd', 0x14, 0x65,
                                           'n', 'o', 'n', 'c', 'e', 0x13});

    test_encoder subject{};

    test_output_stream ostream{};

    custom_with_named_layout_descriptor t{0x13, 0x07, 0x04, 0x14};
    auto rx = subject(ostream, t);
    DPLX_REQUIRE_RESULT(rx);

    BOOST_TEST(byte_span(bytes) == byte_span(ostream),
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
