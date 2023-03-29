
// Copyright 2023 Henrik Steffen Gaßmann
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/state.hpp"

#include <catch2/catch_test_macros.hpp>

#include "test_utils.hpp"

namespace dp_tests
{

using namespace cncr::uuid_literals;

TEST_CASE("state_map is default constructible")
{
    dp::state_store states;
    CHECK(states.empty());
}

TEST_CASE("can insert a simple state into state_map")
{
    constexpr dp::state_key<int> k{"fdd790a7-c618-4e58-b412-8042b42bd9bb"_uuid};
    dp::state_store states;

    auto const value = 4;
    auto [state, inserted] = states.emplace(k, value);
    CHECK(inserted);
    CHECK(*state == 4);

    auto *accessed = states.try_access(k);
    CHECK(state == accessed);
}

TEST_CASE("scoped_state manages a state lifetime")
{
    constexpr dp::state_key<int> k{"fdd790a7-c618-4e58-b412-8042b42bd9bb"_uuid};
    dp::state_store states;

    CHECK(states.try_access(k) == nullptr);
    {
        dp::scoped_state stateScope(states, k);
        REQUIRE(stateScope.get() != nullptr);
        CHECK(*stateScope.get() == int{});
    }
    CHECK(states.try_access(k) == nullptr);
}

TEST_CASE("link_map is default constructible")
{
    dp::link_store links;
    CHECK(links.empty());
}

TEST_CASE("can insert a simple link into link_map")
{
    constexpr dp::state_link_key<int> k{
            "fdd790a7-c618-4e58-b412-8042b42bd9bb"_uuid};
    dp::link_store links;

    auto const value = 4;
    CHECK(links.replace(k, value) == int{});

    CHECK(value == links.try_access(k));
}

TEST_CASE("scoped_link manages a link lifetime")
{
    constexpr dp::state_link_key<int> k{
            "fdd790a7-c618-4e58-b412-8042b42bd9bb"_uuid};
    dp::link_store links;

    CHECK(links.try_access(k) == int{});
    {
        auto const value = 4;
        dp::scoped_link linkScope(links, k, value);
        CHECK(links.try_access(k) == value);
    }
    CHECK(links.try_access(k) == int{});
}

} // namespace dp_tests
