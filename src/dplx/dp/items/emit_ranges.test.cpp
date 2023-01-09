
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/emit_ranges.hpp"

#include <algorithm>
#include <forward_list>
#include <numeric>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <dplx/dp/indefinite_range.hpp>
#include <dplx/dp/map_pair.hpp>
#include <dplx/dp/streams/memory_output_stream2.hpp>

#include "item_sample_ct.hpp"
#include "range_generator.hpp"
#include "simple_encodable.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

namespace
{
constexpr item_sample_ct<unsigned> array_samples[] = {
        { 0,  1,                          {0x80}},
        { 1,  2,                       {0x81, 1}},
        {23, 24,  {0x97, 1, 2, 3, 4, 5, 6, 7, 8}},
        {24, 26, {0x98, 24, 1, 2, 3, 4, 5, 6, 7}},
};
}

TEST_CASE("emit_array loops over a sized range of encodables and emits them")
{
    item_sample_ct<unsigned> const sample
            = GENERATE(borrowed_range(array_samples));

    std::vector<simple_encodable> generated(sample.value);
    std::iota(generated.begin(), generated.end(), simple_encodable{1});

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);
    dp::emit_context const ctx{outputStream};

    REQUIRE(emit_array(ctx, generated));

    REQUIRE(outputStream.written().size() == sample.encoded_length);
    auto const prefix_length
            = std::min(outputStream.written().size(), sample.encoded.size());
    CHECK(std::ranges::equal(std::span(buffer).first(prefix_length),
                             sample.encoded_bytes()));
}

TEST_CASE("emit_array loops over a forward range of encodables and emits them")
{
    item_sample_ct<unsigned> const sample
            = GENERATE(borrowed_range(array_samples));

    std::forward_list<simple_encodable> generated(sample.value);
    std::iota(generated.begin(), generated.end(), simple_encodable{1});

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);
    dp::emit_context const ctx{outputStream};

    REQUIRE(emit_array(ctx, dp::indefinite_range(generated)));

    REQUIRE(outputStream.written().size() == sample.encoded_length);
    auto const prefix_length
            = std::min(outputStream.written().size(), sample.encoded.size());
    CHECK(std::ranges::equal(std::span(buffer).first(prefix_length),
                             sample.encoded_bytes()));
}

TEST_CASE("emit_array can handle various data types")
{
    constexpr simple_encodable sample[] = {{0b1110'1001}, {0b1110'1001}};
    constexpr std::array expected{to_byte(dp::type_code::array) | std::byte{2U},
                                  std::byte{sample[0].value},
                                  std::byte{sample[1].value}};

    std::vector<std::byte> buffer(3U);
    dp::memory_output_stream outputStream(buffer);
    dp::emit_context const ctx{outputStream};

    SECTION("standard C-Array")
    {
        REQUIRE(emit_array(ctx, sample));
    }
    SECTION("std array")
    {
        constexpr std::array<simple_encodable, 2> input{sample[0], sample[1]};
        REQUIRE(emit_array(ctx, input));
    }
    CHECK(std::ranges::equal(std::span(buffer), expected));
}

namespace
{
constexpr item_sample_ct<unsigned> indefinite_array_samples[] = {
        { 0,  2,                   {0x9f, 0xff}},
        { 1,  3,                {0x9f, 1, 0xff}},
        {23, 25, {0x9f, 1, 2, 3, 4, 5, 6, 7, 8}},
        {24, 26, {0x9f, 1, 2, 3, 4, 5, 6, 7, 8}},
};
}

TEST_CASE("emit_array_indefinite loops over an input range of encodables and "
          "emits them")
{
    item_sample_ct<unsigned> const sample
            = GENERATE(borrowed_range(indefinite_array_samples));

    std::forward_list<simple_encodable> generated(sample.value);
    std::iota(generated.begin(), generated.end(), simple_encodable{1});

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);
    dp::emit_context const ctx{outputStream};

    REQUIRE(emit_array_indefinite(ctx, dp::indefinite_range(generated)));

    REQUIRE(outputStream.written().size() == sample.encoded_length);
    auto const prefix_length
            = std::min(outputStream.written().size(), sample.encoded.size());
    CHECK(std::ranges::equal(std::span(buffer).first(prefix_length),
                             sample.encoded_bytes()));
}

namespace
{
constexpr item_sample_ct<unsigned> map_samples[] = {
        { 0,  1,  {0xa0, 0, 0, 0, 0, 0, 0, 0, 0}},
        { 1,  3,  {0xa1, 0, 1, 0, 0, 0, 0, 0, 0}},
        {23, 47,  {0xb7, 0, 1, 1, 2, 2, 3, 3, 4}},
        {24, 50, {0xb8, 24, 0, 1, 1, 2, 2, 3, 3}},
};
}

TEST_CASE("emit_map loops over a sized range of encodable pairs and emits them")
{
    item_sample_ct<unsigned> const sample
            = GENERATE(borrowed_range(map_samples));

    // clang won't support ranges::views till version 16
    std::vector<simple_encodable> generated(sample.value);
    std::iota(generated.begin(), generated.end(), simple_encodable{0});
    std::vector<dp::map_pair<simple_encodable, simple_encodable>> enlarged(
            sample.value);
    std::ranges::transform(
            generated, enlarged.begin(),
            [](simple_encodable o)
                    -> dp::map_pair<simple_encodable, simple_encodable> {
                return {o, ++o};
            });

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);
    dp::emit_context const ctx{outputStream};

    REQUIRE(emit_map(ctx, enlarged));

    REQUIRE(outputStream.written().size() == sample.encoded_length);
    auto const prefix_length
            = std::min(outputStream.written().size(), sample.encoded.size());
    CHECK(std::ranges::equal(std::span(buffer).first(prefix_length),
                             sample.encoded_bytes()));
}

TEST_CASE(
        "emit_map loops over a forward range of encodable pairs and emits them")
{
    item_sample_ct<unsigned> const sample
            = GENERATE(borrowed_range(map_samples));

    // clang won't support ranges::views till version 16
    std::vector<simple_encodable> generated(sample.value);
    std::iota(generated.begin(), generated.end(), simple_encodable{0});
    std::forward_list<dp::map_pair<simple_encodable, simple_encodable>>
            enlarged(sample.value);
    std::ranges::transform(
            generated, enlarged.begin(),
            [](simple_encodable o)
                    -> dp::map_pair<simple_encodable, simple_encodable> {
                return {o, ++o};
            });

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);
    dp::emit_context const ctx{outputStream};

    REQUIRE(emit_map(ctx, dp::indefinite_range(enlarged)));

    REQUIRE(outputStream.written().size() == sample.encoded_length);
    auto const prefix_length
            = std::min(outputStream.written().size(), sample.encoded.size());
    CHECK(std::ranges::equal(std::span(buffer).first(prefix_length),
                             sample.encoded_bytes()));
}

namespace
{
constexpr item_sample_ct<unsigned> indefinite_map_samples[] = {
        { 0,  2, {0xbf, 0xFF, 0, 0, 0, 0, 0, 0, 0}},
        { 1,  4, {0xbf, 0, 1, 0xFF, 0, 0, 0, 0, 0}},
        {23, 48,    {0xbf, 0, 1, 1, 2, 2, 3, 3, 4}},
        {24, 50,    {0xbf, 0, 1, 1, 2, 2, 3, 3, 4}},
};
}

TEST_CASE("emit_map_indefinite loops over an input range of encodable pairs "
          "and emits them")
{
    item_sample_ct<unsigned> const sample
            = GENERATE(borrowed_range(indefinite_map_samples));

    // clang won't support ranges::views till version 16
    std::vector<simple_encodable> generated(sample.value);
    std::iota(generated.begin(), generated.end(), simple_encodable{0});
    std::forward_list<dp::map_pair<simple_encodable, simple_encodable>>
            enlarged(sample.value);
    std::ranges::transform(
            generated, enlarged.begin(),
            [](simple_encodable o)
                    -> dp::map_pair<simple_encodable, simple_encodable> {
                return {o, ++o};
            });

    std::vector<std::byte> buffer(sample.encoded_length);
    dp::memory_output_stream outputStream(buffer);
    dp::emit_context const ctx{outputStream};

    REQUIRE(emit_map_indefinite(ctx, dp::indefinite_range(enlarged)));

    REQUIRE(outputStream.written().size() == sample.encoded_length);
    auto const prefix_length
            = std::min(outputStream.written().size(), sample.encoded.size());
    CHECK(std::ranges::equal(std::span(buffer).first(prefix_length),
                             sample.encoded_bytes()));
}

} // namespace dp_tests
