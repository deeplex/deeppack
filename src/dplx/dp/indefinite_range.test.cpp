
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/indefinite_range.hpp"

#include <array>
#include <type_traits>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "test_utils.hpp"

namespace dp_tests
{

static_assert(std::ranges::range<dp::indefinite_range<int *>>);
static_assert(std::ranges::view<dp::indefinite_range<int *>>);
static_assert(std::ranges::borrowed_range<dp::indefinite_range<int *>>);

TEST_CASE("indefinite_range can wrap std::array")
{
    std::array const source{0, 1, 2, 3};
    dp::indefinite_range const indefinite(source);
    static_assert(
            !dp::enable_indefinite_encoding<std::decay_t<decltype(source)>>);
    static_assert(
            dp::enable_indefinite_encoding<std::decay_t<decltype(indefinite)>>);

    CHECK(indefinite.begin() == source.begin());
    CHECK(indefinite.end() == source.end());
}

TEST_CASE("indefinite_range can wrap std::array iterators")
{
    std::array const source{0, 1, 2, 3};
    dp::indefinite_range const indefinite(source.begin(), source.end());
    static_assert(
            !dp::enable_indefinite_encoding<std::decay_t<decltype(source)>>);
    static_assert(
            dp::enable_indefinite_encoding<std::decay_t<decltype(indefinite)>>);

    CHECK(indefinite.begin() == source.begin());
    CHECK(indefinite.end() == source.end());
}

TEST_CASE("indefinite_range can wrap std::vector")
{
    std::vector const source{0, 1, 2, 3};
    dp::indefinite_range const indefinite(source);
    static_assert(
            !dp::enable_indefinite_encoding<std::decay_t<decltype(source)>>);
    static_assert(
            dp::enable_indefinite_encoding<std::decay_t<decltype(indefinite)>>);

    CHECK(indefinite.begin() == source.begin());
    CHECK(indefinite.end() == source.end());
}

TEST_CASE("indefinite_range can wrap std::vector iterators")
{
    std::vector const source{0, 1, 2, 3};
    dp::indefinite_range const indefinite(source.begin(), source.end());
    static_assert(
            !dp::enable_indefinite_encoding<std::decay_t<decltype(source)>>);
    static_assert(
            dp::enable_indefinite_encoding<std::decay_t<decltype(indefinite)>>);

    CHECK(indefinite.begin() == source.begin());
    CHECK(indefinite.end() == source.end());
}

} // namespace dp_tests
