
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/map_pair.hpp"

#include <catch2/catch_test_macros.hpp>

#include <dplx/dp/detail/type_utils.hpp>

#include "test_utils.hpp"

namespace dp_tests
{

static_assert(dp::pair_like<dp::map_pair<int, int>>);
static_assert(std::is_trivial_v<dp::map_pair<int, int>>);

TEST_CASE("map_pair can be constructed with CTAD")
{
    dp::map_pair subject{1, 2L};
    static_assert(std::same_as<std::decay_t<decltype(subject)>,
                               dp::map_pair<int, long>>);

    CHECK(subject.key == 1);
    CHECK(subject.value == 2L);
    CHECK(get<0>(subject) == 1);
    CHECK(get<1>(subject) == 2L);
}

} // namespace dp_tests
