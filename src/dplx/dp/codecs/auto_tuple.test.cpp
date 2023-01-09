
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/auto_tuple.hpp"

#include <catch2/catch_test_macros.hpp>

#include "dplx/dp/codecs/core.hpp"
#include "dplx/dp/streams/memory_output_stream2.hpp"
#include "dplx/dp/streams/void_stream.hpp"
#include "item_sample_ct.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

struct test_tuple
{
    std::uint32_t ma;
    std::uint64_t mb;
    std::uint32_t mc;
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
            {0xdeadbeefU, 0x07U, 0xfefeU},
            6,
            {0x81, 0x1a, 0xde, 0xad, 0xbe, 0xef}
    };

    SECTION("can encode")
    {
        std::vector<std::byte> encodingBuffer(sample.encoded_length);
        dp::memory_output_stream outputStream(encodingBuffer);
        dp::emit_context ctx{outputStream};

        REQUIRE(dp::encode_tuple<descriptor>(ctx, sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_tuple<descriptor>(ctx, sample.value));
    }
}

TEST_CASE("encode tuple with layout descriptor 2")
{
    constexpr auto const &descriptor = test_tuple_def_2;
    item_sample_ct<test_tuple> const sample{
            {0xdeadbeafU, 0x07U, 0xfefeU},
            7,
            {0x82, 0x1a, 0xde, 0xad, 0xbe, 0xaf, 0x07}
    };

    SECTION("can encode")
    {
        std::vector<std::byte> encodingBuffer(sample.encoded_length);
        dp::memory_output_stream outputStream(encodingBuffer);
        dp::emit_context ctx{outputStream};

        REQUIRE(dp::encode_tuple<descriptor>(ctx, sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_tuple<descriptor>(ctx, sample.value));
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
        std::vector<std::byte> encodingBuffer(sample.encoded_length);
        dp::memory_output_stream outputStream(encodingBuffer);
        dp::emit_context ctx{outputStream};

        REQUIRE(dp::encode_tuple<descriptor>(ctx, sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_tuple<descriptor>(ctx, sample.value));
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
        std::vector<std::byte> encodingBuffer(sample.encoded_length);
        dp::memory_output_stream outputStream(encodingBuffer);
        dp::emit_context ctx{outputStream};

        REQUIRE(dp::encode_tuple<descriptor>(ctx, sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_tuple<descriptor>(ctx, sample.value));
    }
}

TEST_CASE("encode tuple with layout descriptor 5")
{
    constexpr auto const &descriptor = test_tuple_def_5;
    item_sample_ct<test_tuple> const sample{
            {0xdeadbeafU, 0x07U, 0xfefeU},
            7,
            {0x82, 0x07, 0x1a, 0xde, 0xad, 0xbe, 0xaf}
    };

    SECTION("can encode")
    {
        std::vector<std::byte> encodingBuffer(sample.encoded_length);
        dp::memory_output_stream outputStream(encodingBuffer);
        dp::emit_context ctx{outputStream};

        REQUIRE(dp::encode_tuple<descriptor>(ctx, sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_tuple<descriptor>(ctx, sample.value));
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
        std::vector<std::byte> encodingBuffer(sample.encoded_length);
        dp::memory_output_stream outputStream(encodingBuffer);
        dp::emit_context ctx{outputStream};

        REQUIRE(dp::encode_tuple(ctx, sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_tuple(ctx, sample.value));
    }
}

} // namespace dp_tests
