
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/auto_object.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "blob_matcher.hpp"
#include "dplx/dp/codecs/core.hpp"
#include "dplx/dp/streams/memory_output_stream2.hpp"
#include "dplx/dp/streams/void_stream.hpp"
#include "item_sample_ct.hpp"
#include "range_generator.hpp"
#include "test_input_stream.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

// NOLINTBEGIN(readability-function-cognitive-complexity)

namespace dp_tests
{

namespace
{

constexpr item_sample_ct<dp::object_head_info> object_head_samples[] = {
        {{3, dp::null_def_version}, 7, {0xa3, 1, 1}},
        {            {0x04 - 1, 0}, 9, {0xa4, 0, 0}},
        {            {0x04 - 1, 5}, 9, {0xa4, 0, 5}},
};

}

TEST_CASE("decode_object_head parses a map item head")
{
    auto const sample = GENERATE(borrowed_range(object_head_samples));

    simple_test_parse_context ctx(sample.encoded_bytes());
    auto rx = dp::decode_object_head(ctx.as_parse_context(), std::true_type{});
    REQUIRE(rx);

    CHECK(rx.assume_value() == sample.value);
}

namespace
{

struct test_object
{
    std::uint64_t ma;
    std::uint32_t mb;
    std::uint32_t mc;

    friend inline auto operator==(test_object const &,
                                  test_object const &) noexcept -> bool
            = default;
};

constexpr dp::object_def<dp::property_def<1, &test_object::ma>{}>
        test_object_def_1{};
static_assert(!test_object_def_1.has_optional_properties);

constexpr dp::object_def<dp::property_def<1, &test_object::ma>{},
                         dp::property_def<23, &test_object::mb>{}>
        test_object_def_2{};
static_assert(!test_object_def_2.has_optional_properties);

constexpr dp::object_def<dp::property_def<1, &test_object::ma>{},
                         dp::property_def<23, &test_object::mb>{},
                         dp::property_def<64, &test_object::mc>{}>
        test_object_def_3{};
static_assert(!test_object_def_3.has_optional_properties);

constexpr dp::object_def<dp::property_def<1, &test_object::ma>{},
                         dp::property_def<23, &test_object::mb>{false},
                         dp::property_def<64, &test_object::mc>{}>
        test_object_def_3_with_optional{};
static_assert(test_object_def_3_with_optional.has_optional_properties);

} // namespace

// TODO: port the following auto object tests
/*
namespace
{

constexpr item_sample_ct<test_object> single_property_samples[] = {
        {    {.ma = 23U, .mb = 0U, .mc = 0U}, 2,                      {1, 23}},
        {  {.ma = 0U, .mb = 0xefU, .mc = 0U}, 2,            {23, 0x018, 0xfe}},
        {{.ma = 0U, .mb = 0U, .mc = 0xdeadU}, 2, {0x18, 64, 0x1a, 0xde, 0xad}},
};

}

TEST_CASE("decode a single object property")
{
    auto const sample = GENERATE(borrowed_range(single_property_samples));

    simple_test_parse_context ctx(sample.encoded_bytes());

    std::remove_cvref_t<decltype(sample.value)> value{};
    REQUIRE(dp::decode_object_property<test_object_def_3>(ctx, value));

    CHECK(value == sample.value);
}

BOOST_AUTO_TEST_CASE(prop_decode_3)
{
    auto bytes = make_byte_array<32>({0x18, 64, 0x1A, 0xDE, 0xAD, 0xBE, 0xEF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dp::decode_object_property<test_tuple_def_3>(istream, out);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0U);
    BOOST_TEST(out.mb == 0U);
    BOOST_TEST(out.mc == 0xDEADBEEFU);
}

BOOST_AUTO_TEST_CASE(prop_decode_reject_unknown_prop)
{
    auto bytes = make_byte_array<32>({0x18, 65, 0x11});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dp::decode_object_property<test_tuple_def_3>(istream, out);
    BOOST_TEST(rx.error() == errc::unknown_property);
}

BOOST_AUTO_TEST_CASE(prop_decode_reject_invalid_id_type)
{
    auto bytes = make_byte_array<32>({0x20, 0x11});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dp::decode_object_property<test_tuple_def_3>(istream, out);
    BOOST_TEST(rx.error() == errc::item_type_mismatch);
}

BOOST_AUTO_TEST_CASE(def_1)
{
    auto bytes = make_byte_array<32>({1, 0x17});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dp::decode_object_properties<test_tuple_def_1>(istream, out, 1);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0x17U);
    BOOST_TEST(out.mb == 0U);
    BOOST_TEST(out.mc == 0U);
}

BOOST_AUTO_TEST_CASE(def_2)
{
    auto bytes = make_byte_array<32>({23, 0x18, 0xEF, 1, 0x19, 0x48, 0xCF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dp::decode_object_properties<test_tuple_def_2>(istream, out, 2);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0x48CFU);
    BOOST_TEST(out.mb == 0xEFU);
    BOOST_TEST(out.mc == 0U);
}

BOOST_AUTO_TEST_CASE(def_3)
{
    auto bytes = make_byte_array<32>({23, 0x18, 0xEF, 1, 0x19, 0x48, 0xCF, 0x18,
                                      64, 0x1A, 0xDE, 0xAD, 0xBE, 0xEF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dp::decode_object_properties<test_tuple_def_3>(istream, out, 3);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0x48CFU);
    BOOST_TEST(out.mb == 0xEFU);
    BOOST_TEST(out.mc == 0xDEADBEEFU);
}

BOOST_AUTO_TEST_CASE(def_3_reject_missing_property)
{
    auto bytes = make_byte_array<32>({23, 0x18, 0xEF, 1, 0x19, 0x48, 0xCF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dp::decode_object_properties<test_tuple_def_3>(istream, out, 2);
    BOOST_TEST(rx.error() == errc::required_object_property_missing);
}

BOOST_AUTO_TEST_CASE(def_3_with_optional_set)
{
    auto bytes = make_byte_array<32>({23, 0x18, 0xEF, 1, 0x19, 0x48, 0xCF, 0x18,
                                      64, 0x1A, 0xDE, 0xAD, 0xBE, 0xEF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dp::decode_object_properties<test_tuple_def_3_with_optional>(
            istream, out, 3);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0x48CFU);
    BOOST_TEST(out.mb == 0xEFU);
    BOOST_TEST(out.mc == 0xDEADBEEFU);
}

BOOST_AUTO_TEST_CASE(def_3_with_optional_not_set)
{
    auto bytes = make_byte_array<32>(
            {0x18, 64, 0x1A, 0xDE, 0xAD, 0xBE, 0xEF, 1, 0x19, 0x48, 0xCF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dp::decode_object_properties<test_tuple_def_3_with_optional>(
            istream, out, 2);
    DPLX_REQUIRE_RESULT(rx);
    BOOST_TEST(out.ma == 0x48CFU);
    BOOST_TEST(out.mb == 0U);
    BOOST_TEST(out.mc == 0xDEADBEEFU);
}

BOOST_AUTO_TEST_CASE(def_3_with_optional_reject_missing_property)
{
    auto bytes = make_byte_array<32>({23, 0x18, 0xEF, 1, 0x19, 0x48, 0xCF});
    test_input_stream istream{bytes};

    test_tuple out{};
    auto rx = dp::decode_object_properties<test_tuple_def_3_with_optional>(
            istream, out, 2);
    BOOST_TEST(rx.error() == errc::required_object_property_missing);
}

*/

TEST_CASE("object codec helpers with layout descriptor")
{
    constexpr auto const &descriptor = test_object_def_2;
    item_sample_ct<test_object> const sample{
            {0xdeadbeafU, 0x07U, 0xfefeU},
            9,
            {0xa2, 0x01, 0x1a, 0xde, 0xad, 0xbe, 0xaf, 0x17, 0x07}
    };

    SECTION("can encode")
    {
        simple_test_emit_context ctx(sample.encoded_length);

        REQUIRE(dp::encode_object<descriptor>(ctx.as_emit_context(),
                                              sample.value));

        CHECK_BLOB_EQ(ctx.stream.written(), sample.encoded_bytes());
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_object<descriptor>(ctx, sample.value));
    }
    SECTION("with decode")
    {
        simple_test_parse_context ctx(sample.encoded_bytes());

        auto decodeHeadRx = dp::decode_object_head(ctx.as_parse_context());
        REQUIRE(decodeHeadRx);
        auto const &head = decodeHeadRx.assume_value();

        CHECK(head.version == descriptor.version);
        CHECK(head.num_properties == descriptor.num_properties);

        std::remove_cvref_t<decltype(sample.value)> value{};
        REQUIRE(dp::decode_object_properties<descriptor>(
                ctx.as_parse_context(), value, head.num_properties));

        CHECK(value
              == test_object{
                      .ma = sample.value.ma,
                      .mb = sample.value.mb,
                      .mc = 0U,
              });
    }
}

namespace
{

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

    static constexpr dp::object_def<
            dp::property_def<1, &custom_with_layout_descriptor::ma>{},
            dp::property_def<2, &custom_with_layout_descriptor::mb>{},
            dp::property_def<5,
                             &custom_with_layout_descriptor::msub,
                             &test_object::mc>{},
            dp::property_fun<26, mc_accessor>{},
            dp::property_def<36, &custom_with_layout_descriptor::md>{}>
            layout_descriptor{};

    [[nodiscard]] auto a() const noexcept -> std::uint64_t
    {
        return ma;
    }
    [[nodiscard]] auto b() const noexcept -> std::uint32_t
    {
        return mb;
    }
    [[nodiscard]] auto c() const noexcept -> std::uint32_t
    {
        return mc;
    }
    [[nodiscard]] auto d() const noexcept -> std::uint32_t
    {
        return md;
    }
    [[nodiscard]] auto msubmc() const noexcept -> std::uint32_t
    {
        return msub.mc;
    }
};

} // namespace

static_assert(dp::packable_object<custom_with_layout_descriptor>);

TEST_CASE("object codec helpers with auto numeric layout descriptor")
{
    item_sample_ct<custom_with_layout_descriptor, 16> const sample{
            {0x13, 0x07, 0x04, 0x14, {0, 0, 0xcfU}},
            14,
            {0xa5, 1, 0x13, 2, 7, 5, 0x18, 0xcfU, 0x18, 26, 4, 0x18, 36, 0x14}
    };

    SECTION("can encode")
    {
        simple_test_emit_context ctx{sample.encoded_length};

        REQUIRE(dp::encode_object(ctx.as_emit_context(), sample.value));

        CHECK_BLOB_EQ(ctx.stream.written(), sample.encoded_bytes());
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_object(ctx, sample.value));
    }
    SECTION("with decode")
    {
        simple_test_parse_context ctx(sample.encoded_bytes());

        std::remove_cvref_t<decltype(sample.value)> value{};
        REQUIRE(dp::decode_object(ctx.as_parse_context(), value));

        CHECK(value.a() == 0x13U);
        CHECK(value.b() == 0x07U);
        CHECK(value.c() == 0x04U);
        CHECK(value.d() == 0x14U);
        CHECK(value.msubmc() == 0xcfU);
    }
}

namespace
{

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

    friend constexpr auto
    operator==(custom_with_named_layout_descriptor const &,
               custom_with_named_layout_descriptor const &) noexcept -> bool
            = default;

    static constexpr dp::object_def<
            dp::named_property_def<u8"b",
                                   &custom_with_named_layout_descriptor::mb>{},
            dp::named_property_def<u8"c",
                                   &custom_with_named_layout_descriptor::mc>{},
            dp::named_property_def<u8"d",
                                   &custom_with_named_layout_descriptor::md>{},
            dp::named_property_def<u8"nonce",
                                   &custom_with_named_layout_descriptor::ma>{}>
            layout_descriptor{};

    [[nodiscard]] auto a() const noexcept -> std::uint64_t
    {
        return ma;
    }
    [[nodiscard]] auto b() const noexcept -> std::uint32_t
    {
        return mb;
    }
    [[nodiscard]] auto c() const noexcept -> std::uint32_t
    {
        return mc;
    }
    [[nodiscard]] auto d() const noexcept -> std::uint32_t
    {
        return md;
    }
};

} // namespace

static_assert(dp::packable_object<custom_with_named_layout_descriptor>);

TEST_CASE("object codec helpers with auto named layout descriptor")
{
    item_sample_ct<custom_with_named_layout_descriptor, 32> const sample{
            {0x13, 0x07, 0x04, 0x14},
            17,
            {0xa4, 0x61, 'b', 7, 0x61, 'c', 0x04, 0x61, 'd', 0x14, 0x65, 'n',
             'o', 'n', 'c', 'e', 0x13}
    };

    SECTION("can encode")
    {
        simple_test_emit_context ctx{sample.encoded_length};

        REQUIRE(dp::encode_object(ctx.as_emit_context(), sample.value));

        CHECK_BLOB_EQ(ctx.stream.written(), sample.encoded_bytes());
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_object(ctx, sample.value));
    }
    SECTION("can decode")
    {
        simple_test_parse_context ctx{sample.encoded_bytes()};

        std::remove_cvref_t<decltype(sample.value)> value;
        REQUIRE(dp::decode_object(ctx.as_parse_context(), value));

        CHECK(value == sample.value);
    }
}

} // namespace dp_tests

// NOLINTEND(readability-function-cognitive-complexity)
