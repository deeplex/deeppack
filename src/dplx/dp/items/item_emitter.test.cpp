
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/item_emitter.hpp"

#include <array>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

#include <dplx/predef/compiler.h>

#include "dplx/dp/detail/workaround.hpp"
#include "dplx/dp/streams/memory_output_stream2.hpp"
#include "range_generator.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

namespace
{

template <typename T>
struct item_sample
{
    T value;
    unsigned encoded_length;
    std::array<std::uint8_t, dp::detail::var_uint_max_size> encoded;

    template <typename U>
    [[nodiscard]] constexpr auto as() const noexcept -> item_sample<U>
    {
        return {static_cast<U>(value), encoded_length, encoded};
    }

    [[nodiscard]] auto encoded_bytes() const -> std::span<std::byte const>
    {
        return as_bytes(std::span(encoded)).first(encoded_length);
    }

    friend inline auto operator<<(std::ostream &os, item_sample const &sample)
            -> std::ostream &
    {
        fmt::print(os, "{{item_value: <please specialize me>, {}, 0x{:02x}}}",
                   sample.encoded_length,
                   fmt::join(sample.encoded_bytes(), "'"));
        return os;
    }
    friend inline auto operator<<(std::ostream &os, item_sample const &sample)
            -> std::ostream &requires(std::integral<T>)
    {
        fmt::print(os, "{{item_value: {:#x}, {}, 0x{:02x}}}", sample.value,
                   sample.encoded_length,
                   fmt::join(sample.encoded_bytes(), "'"));
        return os;
    }
    friend inline auto operator<<(std::ostream &os, item_sample const &sample)
            -> std::ostream &requires(std::floating_point<T>)
    {
        fmt::print(os, "{{item_value: {}, {}, 0x{:02x}}}", sample.value,
                   sample.encoded_length,
                   fmt::join(sample.encoded_bytes(), "'"));
        return os;
    }
};

} // namespace

TEST_CASE("boolean emits correctly")
{
    auto sample = GENERATE(gens::values<item_sample<bool>>({
            {false, 1, {0b111'10100}},
            { true, 1, {0b111'10101}},
    }));

    std::vector<std::byte> encodingBuffer(sample.encoded_length);
    dp::memory_output_stream outputStream(encodingBuffer);

    dp::ng::item_emitter const emit(outputStream);
    REQUIRE(emit.boolean(sample.value));

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

namespace
{

template <typename T>
using limits = std::numeric_limits<T>;

template <typename R, typename T, std::size_t N>
auto integer_samples(item_sample<T> const (&samples)[N])
{
#if DPLX_DP_WORKAROUND_TESTED_AT(DPLX_COMP_CLANG, 15, 0, 0)
    typename iterator_generator<item_sample<R>>::container_type values;
    values.reserve(N);
    for (auto &sample : samples)
    {
        if (std::in_range<R>(sample.value))
        {
            values.push_back(sample.template as<R>());
        }
    }

    return dp_tests::from_range(std::move(values));
#else
    using namespace std::ranges::views;

    return dp_tests::from_range(
            samples
            | filter([](item_sample<T> const &sample)
                     { return std::in_range<R>(sample.value); })
            | transform([](item_sample<T> const &sample)
                        { return sample.template as<R>(); }));
#endif
}

} // namespace

namespace
{

constexpr item_sample<unsigned long long> posint_samples[] = {
  // Appendix A.Examples
        {        0x00, 1, {0b000'00000}},
        {                 0x01, 1,                          {0b000'00001}},
        {                 0x0a, 1,                          {0b000'01010}},
        {                 0x19, 2,                           {0x18, 0x19}},
        {                 0x64, 2,                           {0x18, 0x64}},
        {               0x03e8, 3,                     {0x19, 0x03, 0xe8}},
        {          0x000f'4240, 5,         {0x1a, 0x00, 0x0f, 0x42, 0x40}},
        {
         0x0000'00e8'd4a5'1000, 9,
         {0x1b, 0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00},
         },
 // boundary test cases
        {                 0x00, 1,                          {0b000'00000}},
        {                 0x17, 1,                          {0b000'10111}},
        {                 0x18, 2,                           {0x18, 0x18}},
        {                 0x7f, 2,                           {0x18, 0x7f}},
        {                 0x80, 2,                           {0x18, 0x80}},
        {                 0xff, 2,                           {0x18, 0xff}},
        {               0x0100, 3,                     {0x19, 0x01, 0x00}},
        {               0x7fff, 3,                     {0x19, 0x7f, 0xff}},
        {               0x8000, 3,                     {0x19, 0x80, 0x00}},
        {               0xffff, 3,                     {0x19, 0xff, 0xff}},
        {          0x0001'0000, 5,         {0x1a, 0x00, 0x01, 0x00, 0x00}},
        {          0x7fff'ffff, 5,         {0x1a, 0x7f, 0xff, 0xff, 0xff}},
        {          0x8000'0000, 5,         {0x1a, 0x80, 0x00, 0x00, 0x00}},
        {          0xffff'ffff, 5,         {0x1a, 0xff, 0xff, 0xff, 0xff}},
        {
         0x0000'0001'0000'0000, 9,
         {0x1b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00},
         },
        {
         0x7fff'ffff'ffff'ffff, 9,
         {0x1b, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
         },
        {
         0x8000'0000'0000'0000, 9,
         {0x1b, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
         },
        {
         0xffff'ffff'ffff'ffff, 9,
         {0x1b, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
         },
};

} // namespace

TEMPLATE_TEST_CASE("positive integers emit correctly",
                   "",
                   unsigned char,
                   signed char,
                   unsigned short,
                   short,
                   unsigned,
                   int,
                   unsigned long,
                   long,
                   unsigned long long,
                   long long)
{
    auto sample = GENERATE(integer_samples<TestType>(posint_samples));
    INFO(sample);

    SECTION("with a fitting buffer")
    {
        std::vector<std::byte> encodingBuffer(sample.encoded_length);
        dp::memory_output_stream outputStream(encodingBuffer);

        dp::ng::item_emitter const emit(outputStream);
        REQUIRE(emit.integer(sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }

    SECTION("with an oversized buffer")
    {
        std::vector<std::byte> encodingBuffer(dp::detail::var_uint_max_size);
        dp::memory_output_stream outputStream(encodingBuffer);

        dp::ng::item_emitter const emit(outputStream);
        REQUIRE(emit.integer(sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }
}

namespace
{

constexpr item_sample<long long> negint_samples[] = {
  // Appendix A.Examples
        {              -10LL,1,                                           {0b001'01001}},
        {                      -100LL, 2,                                            {0x38, 0x63}},
        {                     -1000LL, 3,                                      {0x39, 0x03, 0xe7}},
 // boundary test cases
        {                     -0x01LL, 1,                                           {0b001'00000}},
        {                 -1 - 0x17LL, 1,                                           {0b001'10111}},
        {                 -1 - 0x18LL, 2,                                            {0x38, 0x18}},
        {                 -1 - 0x7fLL, 2,                                            {0x38, 0x7f}},
        {                 -1 - 0x80LL, 2,                                            {0x38, 0x80}},
        {                 -1 - 0xffLL, 2,                                            {0x38, 0xff}},
        {                -1 - 0x100LL, 3,                                      {0x39, 0x01, 0x00}},
        {               -1 - 0x7fffLL, 3,                                      {0x39, 0x7f, 0xff}},
        {               -1 - 0x8000LL, 3,                                      {0x39, 0x80, 0x00}},
        {               -1 - 0xffffLL, 3,                                      {0x39, 0xff, 0xff}},
        {             -1 - 0x1'0000LL, 5,                          {0x3a, 0x00, 0x01, 0x00, 0x00}},
        {          -1 - 0x7fff'ffffLL, 5,                          {0x3a, 0x7f, 0xff, 0xff, 0xff}},
        {          -1 - 0x8000'0000LL, 5,                          {0x3a, 0x80, 0x00, 0x00, 0x00}},
        {          -1 - 0xffff'ffffLL, 5,                          {0x3a, 0xff, 0xff, 0xff, 0xff}},
        {
         -1 - 0x1'0000'0000LL,
         9, {0x3b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00},
         },
        {
         -1 - 0x7fff'ffff'ffff'ffffLL,
         9, {0x3b, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
         },
};

}

TEMPLATE_TEST_CASE("negative integers emit correctly",
                   "",
                   signed char,
                   short,
                   int,
                   long,
                   long long)
{
    auto sample = GENERATE(integer_samples<TestType>(negint_samples));
    INFO(sample);

    SECTION("with a fitting buffer")
    {
        std::vector<std::byte> encodingBuffer(sample.encoded_length);
        dp::memory_output_stream outputStream(encodingBuffer);

        dp::ng::item_emitter const emit(outputStream);
        REQUIRE(emit.integer(sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }

    SECTION("with an oversized buffer")
    {
        std::vector<std::byte> encodingBuffer(dp::detail::var_uint_max_size);
        dp::memory_output_stream outputStream(encodingBuffer);

        dp::ng::item_emitter const emit(outputStream);
        REQUIRE(emit.integer(sample.value));

        CHECK(std::ranges::equal(outputStream.written(),
                                 sample.encoded_bytes()));
    }
}

TEST_CASE("finite prefixes are emitted correctly")
{
    auto sample = GENERATE(integer_samples<std::size_t>(posint_samples));
    INFO(sample);

    std::vector<std::byte> encodingBuffer(sample.encoded_length);
    dp::memory_output_stream outputStream(encodingBuffer);
    dp::ng::item_emitter const emit(outputStream);

    SECTION("for binary")
    {
        sample.encoded[0] |= static_cast<std::uint8_t>(dp::type_code::binary);
        REQUIRE(emit.binary(sample.value));
    }
    SECTION("for text")
    {
        sample.encoded[0] |= static_cast<std::uint8_t>(dp::type_code::text);
        REQUIRE(emit.u8string(sample.value));
    }
    SECTION("for array")
    {
        sample.encoded[0] |= static_cast<std::uint8_t>(dp::type_code::array);
        REQUIRE(emit.array(sample.value));
    }
    SECTION("for map")
    {
        sample.encoded[0] |= static_cast<std::uint8_t>(dp::type_code::map);
        REQUIRE(emit.map(sample.value));
    }
    SECTION("for tags")
    {
        sample.encoded[0] |= static_cast<std::uint8_t>(dp::type_code::tag);
        REQUIRE(emit.tag(sample.value));
    }

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

TEST_CASE("indefinite prefixes are emitted correctly")
{
    std::vector<std::byte> encodingBuffer(1U);
    dp::memory_output_stream outputStream(encodingBuffer);
    dp::ng::item_emitter const emit(outputStream);

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    std::byte expected{0x1f};
    SECTION("for binary")
    {
        expected |= static_cast<std::byte>(dp::type_code::binary);
        REQUIRE(emit.binary_indefinite());
    }
    SECTION("for text")
    {
        expected |= static_cast<std::byte>(dp::type_code::text);
        REQUIRE(emit.u8string_indefinite());
    }
    SECTION("for array")
    {
        expected |= static_cast<std::byte>(dp::type_code::array);
        REQUIRE(emit.array_indefinite());
    }
    SECTION("for map")
    {
        expected |= static_cast<std::byte>(dp::type_code::map);
        REQUIRE(emit.map_indefinite());
    }

    REQUIRE(outputStream.written().size() == 1U);
    CHECK(*outputStream.written().data() == expected);
}

namespace
{

constexpr item_sample<float> float_single_samples[] = {
        {                 100000.0F, 5, {0xfa, 0x47, 0xc3, 0x50, 0x00}},
        {   3.4028234663852886e+38F, 5, {0xfa, 0x7f, 0x7f, 0xff, 0xff}},
        {                 100000.0F, 5, {0xfa, 0x47, 0xc3, 0x50, 0x00}},
        { limits<float>::infinity(), 5, {0xfa, 0x7f, 0x80, 0x00, 0x00}},
        {limits<float>::quiet_NaN(), 5, {0xfa, 0x7f, 0xc0, 0x00, 0x00}},
        {-limits<float>::infinity(), 5, {0xfa, 0xff, 0x80, 0x00, 0x00}},
};

}

TEST_CASE("float single emits correctly")
{
    auto sample = GENERATE(borrowed_range(float_single_samples));
    INFO(sample);

    std::vector<std::byte> encodingBuffer(sample.encoded_length);
    dp::memory_output_stream outputStream(encodingBuffer);

    dp::ng::item_emitter const emit(outputStream);
    REQUIRE(emit.float_single(sample.value));

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

namespace
{

constexpr item_sample<double> float_double_samples[] = {
        {                        1.1,9, {0xfb, 0x3f, 0xf1, 0x99, 0x99, 0x99, 0x99, 0x99, 0x9a}                                     },
        {                    1.e+300, 9, {0xfb, 0x7e, 0x37, 0xe4, 0x3c, 0x88, 0x00, 0x75, 0x9c}},
        { limits<double>::infinity(),
         9, {0xfb, 0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}                             },
        {limits<double>::quiet_NaN(),
         9, {0xfb, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}                             },
        {-limits<double>::infinity(),
         9, {0xfb, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}                             },
};

}

TEST_CASE("float double emits correctly")
{
    auto sample = GENERATE(borrowed_range(float_double_samples));
    INFO(sample);

    std::vector<std::byte> encodingBuffer(sample.encoded_length);
    dp::memory_output_stream outputStream(encodingBuffer);

    dp::ng::item_emitter const emit(outputStream);
    REQUIRE(emit.float_double(sample.value));

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

TEST_CASE("null emits correctly")
{
    std::vector<std::byte> encodingBuffer(1U);
    dp::memory_output_stream outputStream(encodingBuffer);

    dp::ng::item_emitter const emit(outputStream);
    REQUIRE(emit.null());

    REQUIRE(outputStream.written().size() == 1U);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    REQUIRE(*outputStream.written().data() == std::byte{0b111'10110});
}

TEST_CASE("undefined emits correctly")
{
    std::vector<std::byte> encodingBuffer(1U);
    dp::memory_output_stream outputStream(encodingBuffer);

    dp::ng::item_emitter const emit(outputStream);
    REQUIRE(emit.undefined());

    REQUIRE(outputStream.written().size() == 1U);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    REQUIRE(*outputStream.written().data() == std::byte{0b111'10111});
}

TEST_CASE("break emits correctly")
{
    std::vector<std::byte> encodingBuffer(1U);
    dp::memory_output_stream outputStream(encodingBuffer);

    dp::ng::item_emitter const emit(outputStream);
    REQUIRE(emit.break_());

    REQUIRE(outputStream.written().size() == 1U);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    REQUIRE(*outputStream.written().data() == std::byte{0b111'11111});
}

} // namespace dp_tests
