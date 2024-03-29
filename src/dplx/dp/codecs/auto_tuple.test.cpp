
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/auto_tuple.hpp"

#include <catch2/catch_test_macros.hpp>

#include "blob_matcher.hpp"
#include "dplx/dp/codecs/core.hpp"
#include "dplx/dp/streams/void_stream.hpp"
#include "item_sample_ct.hpp"
#include "test_input_stream.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

// NOLINTBEGIN(readability-function-cognitive-complexity)

namespace dp_tests
{

struct test_tuple
{
    std::uint32_t ma;
    std::uint64_t mb;
    std::uint32_t mc;

    friend inline auto operator==(test_tuple const &,
                                  test_tuple const &) noexcept -> bool
            = default;
};

constexpr dp::tuple_def<dp::tuple_member_def<&test_tuple::ma>{}>
        test_tuple_def_1{};

constexpr dp::tuple_def<dp::tuple_member_def<&test_tuple::ma>{},
                        dp::tuple_member_def<&test_tuple::mb>{}>
        test_tuple_def_2{};

constexpr dp::tuple_def<dp::tuple_member_def<&test_tuple::ma>{},
                        dp::tuple_member_def<&test_tuple::mb>{},
                        dp::tuple_member_def<&test_tuple::mc>{}>
        test_tuple_def_3{};

constexpr dp::tuple_def<dp::tuple_member_def<&test_tuple::ma>{},
                        dp::tuple_member_def<&test_tuple::mc>{},
                        dp::tuple_member_def<&test_tuple::mb>{}>
        test_tuple_def_4{
                .version = 1,
                .allow_versioned_auto_decoder = false,
        };

struct tuple_access_b : dp::member_accessor_base<test_tuple, std::uint64_t>
{
    template <typename T>
        requires std::same_as<std::remove_cvref_t<T>, test_tuple>
    constexpr auto operator()(T &&v) const noexcept
    {
        return &v.mb;
    }
};

constexpr dp::tuple_def<dp::tuple_member_fun<tuple_access_b>{},
                        dp::tuple_member_def<&test_tuple::ma>{}>
        test_tuple_def_5{};

TEST_CASE("encode tuple with layout descriptor 1")
{
    constexpr auto const &descriptor = test_tuple_def_1;
    item_sample_ct<test_tuple> const sample{
            {0xdead'beefU, 0x07U, 0xfefeU},
            6,
            {0x81, 0x1a, 0xde, 0xad, 0xbe, 0xef}
    };

    SECTION("can encode")
    {
        simple_test_emit_context ctx(sample.encoded_length);

        REQUIRE(dp::encode_tuple<descriptor>(ctx.as_emit_context(),
                                             sample.value));

        CHECK_BLOB_EQ(ctx.stream.written(), sample.encoded_bytes());
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_tuple<descriptor>(ctx, sample.value)
              == sample.encoded_length);
    }
    SECTION("can decode")
    {
        simple_test_parse_context ctx(sample.encoded_bytes());

        result<dp::tuple_head_info> headParseRx
                = dp::decode_tuple_head(ctx.as_parse_context());
        REQUIRE(headParseRx);
        auto const &head = headParseRx.assume_value();
        REQUIRE(head.version == descriptor.version);

        std::remove_cvref_t<decltype(sample.value)> value{};
        REQUIRE(dp::decode_tuple_properties<descriptor>(
                ctx.as_parse_context(), value, head.num_properties));

        CHECK(value
              == test_tuple{
                      .ma = sample.value.ma,
                      .mb = {},
                      .mc = {},
              });
    }
}

TEST_CASE("encode tuple with layout descriptor 2")
{
    constexpr auto const &descriptor = test_tuple_def_2;
    item_sample_ct<test_tuple> const sample{
            {0xdead'beafU, 0x07U, 0xfefeU},
            7,
            {0x82, 0x1a, 0xde, 0xad, 0xbe, 0xaf, 0x07}
    };

    SECTION("can encode")
    {
        simple_test_emit_context ctx(sample.encoded_length);

        REQUIRE(dp::encode_tuple<descriptor>(ctx.as_emit_context(),
                                             sample.value));

        CHECK_BLOB_EQ(ctx.stream.written(), sample.encoded_bytes());
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_tuple<descriptor>(ctx, sample.value));
    }
    SECTION("can decode")
    {
        simple_test_parse_context ctx(sample.encoded_bytes());

        result<dp::tuple_head_info> headParseRx
                = dp::decode_tuple_head(ctx.as_parse_context());
        REQUIRE(headParseRx);
        auto const &head = headParseRx.assume_value();
        REQUIRE(head.version == descriptor.version);

        std::remove_cvref_t<decltype(sample.value)> value{};
        REQUIRE(dp::decode_tuple_properties<descriptor>(
                ctx.as_parse_context(), value, head.num_properties));

        CHECK(value
              == test_tuple{
                      .ma = sample.value.ma,
                      .mb = sample.value.mb,
                      .mc = {},
              });
    }
}

TEST_CASE("encode tuple with layout descriptor 3")
{
    constexpr auto const &descriptor = test_tuple_def_3;
    item_sample_ct<test_tuple> const sample{
            {0xdeadU, 0x07U, 0xfefeU},
            8,
            {0x83, 0x19, 0xde, 0xad, 0x07, 0x19, 0xfe, 0xfe}
    };

    SECTION("can encode")
    {
        simple_test_emit_context ctx(sample.encoded_length);

        REQUIRE(dp::encode_tuple<descriptor>(ctx.as_emit_context(),
                                             sample.value));

        CHECK_BLOB_EQ(ctx.stream.written(), sample.encoded_bytes());
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_tuple<descriptor>(ctx, sample.value));
    }
    SECTION("can decode")
    {
        simple_test_parse_context ctx(sample.encoded_bytes());

        result<dp::tuple_head_info> headParseRx
                = dp::decode_tuple_head(ctx.as_parse_context());
        REQUIRE(headParseRx);
        auto const &head = headParseRx.assume_value();
        REQUIRE(head.version == descriptor.version);

        std::remove_cvref_t<decltype(sample.value)> value{};
        REQUIRE(dp::decode_tuple_properties<descriptor>(
                ctx.as_parse_context(), value, head.num_properties));

        CHECK(value == sample.value);
    }
}

TEST_CASE("encode tuple with layout descriptor 4")
{
    constexpr auto const &descriptor = test_tuple_def_4;
    item_sample_ct<test_tuple> const sample{
            {0xdeadU, 0x07U, 0xfefeU},
            9,
            {0x84, 0x01, 0x19, 0xde, 0xad, 0x19, 0xfe, 0xfe, 0x07}
    };

    SECTION("can encode")
    {
        simple_test_emit_context ctx(sample.encoded_length);

        REQUIRE(dp::encode_tuple<descriptor>(ctx.as_emit_context(),
                                             sample.value));

        CHECK_BLOB_EQ(ctx.stream.written(), sample.encoded_bytes());
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_tuple<descriptor>(ctx, sample.value));
    }
    SECTION("can decode")
    {
        simple_test_parse_context ctx(sample.encoded_bytes());

        result<dp::tuple_head_info> headParseRx = dp::decode_tuple_head(
                ctx.as_parse_context(), std::true_type{});
        REQUIRE(headParseRx);
        auto const &head = headParseRx.assume_value();
        REQUIRE(head.version == descriptor.version);

        std::remove_cvref_t<decltype(sample.value)> value{};
        REQUIRE(dp::decode_tuple_properties<descriptor>(
                ctx.as_parse_context(), value, head.num_properties));

        CHECK(value == sample.value);
    }
}

TEST_CASE("encode tuple with layout descriptor 5")
{
    constexpr auto const &descriptor = test_tuple_def_5;
    item_sample_ct<test_tuple> const sample{
            {0xdead'beafU, 0x07U, 0xfefeU},
            7,
            {0x82, 0x07, 0x1a, 0xde, 0xad, 0xbe, 0xaf}
    };

    SECTION("can encode")
    {
        simple_test_emit_context ctx(sample.encoded_length);

        REQUIRE(dp::encode_tuple<descriptor>(ctx.as_emit_context(),
                                             sample.value));

        CHECK_BLOB_EQ(ctx.stream.written(), sample.encoded_bytes());
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_tuple<descriptor>(ctx, sample.value));
    }
    SECTION("can decode")
    {
        simple_test_parse_context ctx(sample.encoded_bytes());

        result<dp::tuple_head_info> headParseRx
                = dp::decode_tuple_head(ctx.as_parse_context());
        REQUIRE(headParseRx);
        auto const &head = headParseRx.assume_value();
        REQUIRE(head.version == descriptor.version);

        std::remove_cvref_t<decltype(sample.value)> value{};
        REQUIRE(dp::decode_tuple_properties<descriptor>(
                ctx.as_parse_context(), value, head.num_properties));

        CHECK(value
              == test_tuple{
                      .ma = sample.value.ma,
                      .mb = sample.value.mb,
                      .mc = {},
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

    test_tuple msub;

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
                                            test_tuple sub)
        : ma(a)
        , mb(b)
        , mc(c)
        , md(d)
        , msub(sub)
    {
    }

    friend inline auto
    operator==(custom_with_layout_descriptor const &,
               custom_with_layout_descriptor const &) noexcept -> bool
            = default;

    static constexpr dp::tuple_def<
            dp::tuple_member_def<&custom_with_layout_descriptor::mb>{},
            dp::tuple_member_def<&custom_with_layout_descriptor::ma>{},
            dp::tuple_member_fun<mc_accessor>{},
            dp::tuple_member_def<&custom_with_layout_descriptor::md>{},
            dp::tuple_member_def<&custom_with_layout_descriptor::msub,
                                 &test_tuple::mb>{}>
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

    [[nodiscard]] auto msubmb() const noexcept -> std::uint64_t
    {
        return msub.mb;
    }
};

} // namespace

static_assert(dp::packable_tuple<custom_with_layout_descriptor>);

TEST_CASE("encode tuple with auto layout descriptor")
{
    item_sample_ct<custom_with_layout_descriptor> const sample{
            {0x09, 0x13, 0x15, 0x17, {0, 0x05, 0}},
            6,
            {0b100'00101, 0x13, 0x09, 0x15, 0x17, 0x05}
    };

    SECTION("can encode")
    {
        simple_test_emit_context ctx(sample.encoded_length);

        REQUIRE(dp::encode_tuple(ctx.as_emit_context(), sample.value));

        CHECK_BLOB_EQ(ctx.stream.written(), sample.encoded_bytes());
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_tuple(ctx, sample.value));
    }
    SECTION("can decode")
    {
        simple_test_parse_context ctx(sample.encoded_bytes());

        std::remove_cvref_t<decltype(sample.value)> value{};
        REQUIRE(dp::decode_tuple(ctx.as_parse_context(), value));

        CHECK(value == sample.value);
    }
}

} // namespace dp_tests

// NOLINTEND(readability-function-cognitive-complexity)
