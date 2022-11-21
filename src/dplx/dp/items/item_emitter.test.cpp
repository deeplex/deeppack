
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/item_emitter.hpp"

#include <array>
#include <vector>

#include <boost/container/static_vector.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

#include <dplx/cncr/misc.hpp>
#include <dplx/cncr/mp_lite.hpp>

#include "dplx/dp/streams/memory_output_stream2.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

namespace
{

using mp_int_type_list = cncr::mp_list<unsigned char,
                                       signed char,
                                       unsigned short,
                                       short,
                                       unsigned,
                                       int,
                                       unsigned long,
                                       long,
                                       unsigned long long,
                                       long long>;

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
        fmt::print(os, "{{item_value: {:#x}, {}, 0x{:02x}}}", sample.value,
                   sample.encoded_length,
                   fmt::join(sample.encoded_bytes(), "'"));
        return os;
    }
};

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
inline constexpr auto num_negint_samples = std::ranges::size(negint_samples);

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
inline constexpr auto num_posint_samples = std::ranges::size(posint_samples);

template <typename T>
class IntegerSampleGenerator
    : public Catch::Generators::IGenerator<item_sample<T>>
{
    std::size_t mIndex{};
    boost::container::static_vector<item_sample<T>,
                                    num_negint_samples + num_posint_samples>
            samples{};

public:
    IntegerSampleGenerator()
    {
        for (auto const &sample : posint_samples)
        {
            if (std::in_range<T>(sample.value))
            {
                samples.push_back(sample.as<T>());
            }
        }
        if constexpr (std::is_signed_v<T>)
        {
            for (auto const &sample : negint_samples)
            {
                if (std::in_range<T>(sample.value))
                {
                    samples.push_back(sample.as<T>());
                }
            }
        }
    }

    [[nodiscard]] auto get() const -> item_sample<T> const & override
    {
        return samples[mIndex];
    }
    [[nodiscard]] auto next() -> bool override
    {
        mIndex += 1;
        return mIndex < samples.size();
    }
};

template <typename T>
auto integer_samples() -> Catch::Generators::GeneratorWrapper<item_sample<T>>
{
    return Catch::Generators::GeneratorWrapper<item_sample<T>>(
            new IntegerSampleGenerator<T>());
}

} // namespace

TEMPLATE_LIST_TEST_CASE("integers emit correctly", "", mp_int_type_list)
{
    auto sample = GENERATE(integer_samples<TestType>());
    INFO(sample);

    std::vector<std::byte> encodingBuffer(sample.encoded_length);
    dp::memory_output_stream outputStream(encodingBuffer);

    dp::ng::item_emitter emit(outputStream);
    REQUIRE(emit.integer(sample.value));

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

TEMPLATE_LIST_TEST_CASE("integers emit correctly presized",
                        "",
                        mp_int_type_list)
{
    auto sample = GENERATE(integer_samples<TestType>());
    INFO(sample);

    std::vector<std::byte> encodingBuffer(dp::detail::var_uint_max_size);
    dp::memory_output_stream outputStream(encodingBuffer);

    dp::ng::item_emitter emit(outputStream);
    REQUIRE(emit.integer(sample.value));

    CHECK(std::ranges::equal(outputStream.written(), sample.encoded_bytes()));
}

} // namespace dp_tests
