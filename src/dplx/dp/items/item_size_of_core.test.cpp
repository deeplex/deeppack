
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/items/item_size_of_core.hpp"

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include "core_samples.hpp"
#include "dplx/dp/streams/void_stream.hpp"
#include "item_sample_ct.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

TEMPLATE_TEST_CASE("positive integers' size is correctly estimated",
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

    dp::void_stream testStream;
    dp::emit_context ctx{testStream};

    CHECK(dp::item_size_of_integer(ctx, sample.value) == sample.encoded_length);
}

TEMPLATE_TEST_CASE("negative integers' size is correctly estimated",
                   "",
                   signed char,
                   short,
                   int,
                   long,
                   long long)
{
    auto sample = GENERATE(integer_samples<TestType>(negint_samples));
    INFO(sample);

    dp::void_stream testStream;
    dp::emit_context ctx{testStream};

    CHECK(dp::item_size_of_integer(ctx, sample.value) == sample.encoded_length);
}

TEST_CASE("binary item size is correctly estimated")
{
    item_sample_ct<unsigned> const sample
            = GENERATE(borrowed_range(binary_samples));

    dp::void_stream testStream;
    dp::emit_context ctx{testStream};

    CHECK(dp::item_size_of_binary(ctx, sample.value) == sample.encoded_length);
    CHECK(dp::item_size_of_binary_indefinite(ctx, sample.value)
          == 1U + sample.value + 1U);
}

TEST_CASE("u8string item size is correctly estimated")
{
    item_sample_ct<std::u8string_view> const sample
            = GENERATE(borrowed_range(u8string_samples));

    dp::void_stream testStream;
    dp::emit_context ctx{testStream};

    CHECK(dp::item_size_of_u8string(ctx, sample.value.size())
          == sample.encoded_length);
    CHECK(dp::item_size_of_u8string_indefinite(ctx, sample.value.size())
          == 1U + sample.value.size() + 1U);
}

} // namespace dp_tests
