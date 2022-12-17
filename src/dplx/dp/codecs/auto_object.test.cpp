
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/auto_object.hpp"

#include <catch2/catch_test_macros.hpp>

#include "dplx/dp/codecs/core.hpp"
#include "dplx/dp/streams/memory_output_stream2.hpp"
#include "dplx/dp/streams/void_stream.hpp"
#include "item_sample.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

namespace
{

struct test_object
{
    std::uint64_t ma;
    std::uint32_t mb;
    std::uint32_t mc;
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

TEST_CASE("object codec helpers with layout descriptor")
{
    constexpr auto const &descriptor = test_object_def_2;
    item_sample<test_object> const sample{
            {0xdeadbeafU, 0x07U, 0xfefeU},
            9,
            {0xa2, 0x01, 0x1a, 0xde, 0xad, 0xbe, 0xaf, 0x17, 0x07}
    };

    SECTION("can encode")
    {
        std::vector<std::byte> encodingBuffer(sample.encoded_length);
        dp::memory_output_stream outputStream(encodingBuffer);
        dp::emit_context ctx{outputStream};

        REQUIRE(dp::encode_object<descriptor>(ctx, sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_object<descriptor>(ctx, sample.value));
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
};

} // namespace

static_assert(dp::packable_object<custom_with_layout_descriptor>);

TEST_CASE("object codec helpers with auto numeric layout descriptor")
{
    item_sample<custom_with_layout_descriptor> const sample{
            {0x13, 0x07, 0x04, 0x14, {0, 0, 0x2a}},
            14,
            {0xa5, 1, 0x13, 2, 7, 5, 0x18, 0x2a, 0x18}
    };

    SECTION("can encode")
    {
        std::vector<std::byte> encodingBuffer(sample.encoded_length);
        dp::memory_output_stream outputStream(encodingBuffer);
        dp::emit_context ctx{outputStream};

        REQUIRE(dp::encode_object(ctx, sample.value));

        auto const encoded
                = std::span(encodingBuffer)
                          .first(std::min<std::size_t>(sample.encoded_length,
                                                       sample.encoded.size()));
        CHECK(std::ranges::equal(encoded, sample.encoded_bytes()));
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_object(ctx, sample.value));
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
    item_sample<custom_with_named_layout_descriptor> const sample{
            {0x13, 0x07, 0x04, 0x14},
            17,
            {0xa4, 0x61, 'b', 7, 0x61, 'c', 0x04, 0x61, 'd'}
    };

    SECTION("can encode")
    {
        std::vector<std::byte> encodingBuffer(sample.encoded_length);
        dp::memory_output_stream outputStream(encodingBuffer);
        dp::emit_context ctx{outputStream};

        REQUIRE(dp::encode_object(ctx, sample.value));

        auto const encoded
                = std::span(encodingBuffer)
                          .first(std::min<std::size_t>(sample.encoded_length,
                                                       sample.encoded.size()));
        CHECK(std::ranges::equal(encoded, sample.encoded_bytes()));
    }
    SECTION("can estimate size")
    {
        dp::void_stream outputStream;
        dp::emit_context ctx{outputStream};
        CHECK(dp::size_of_object(ctx, sample.value));
    }
}

} // namespace dp_tests
