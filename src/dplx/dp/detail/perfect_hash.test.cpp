
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/detail/perfect_hash.hpp"

#include <array>
#include <cstdint>

#include <dplx/dp/detail/hash.hpp>

#include "boost-test.hpp"
#include "test_utils.hpp"

// NOLINTBEGIN(readability-magic-numbers)
// NOLINTBEGIN(readability-implicit-bool-conversion)

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(perfect_hashing)

using dp::detail::perfect_hasher;

struct test_hash
{
    template <cncr::unsigned_integer T>
    constexpr auto operator()(T const value,
                              std::uint64_t const seed = 0) const noexcept
            -> std::uint64_t
    {
        return dp::detail::xxhash3(value, seed);
    }
};

template <typename T, std::size_t N>
constexpr auto failing_hash(std::array<T, N> const &spec) noexcept
        -> std::size_t
{
    perfect_hasher<T, N, test_hash> const ph{spec};

    for (std::size_t i = 0; i < spec.size(); ++i)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        auto const hash = ph(spec[i]);
        if (hash != i)
        {
            return i + 1;
        }
    }
    return 0;
}

BOOST_AUTO_TEST_CASE(h_01n_0)
{
    constexpr std::array subject{0U};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_02_0)
{
    constexpr std::array subject{0U, 1U};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_02_1)
{
    constexpr std::array subject{2U, 4U};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_03_0)
{
    constexpr std::array subject{2U, 3U, 4U};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_03_1)
{
    constexpr std::array subject{2U, 4U, 6U};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_06_0)
{
    constexpr std::array subject{25U, 36U, 37U, 40U, 44U, 46U};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_06_1)
{
    constexpr std::array subject{25U, 36U, 37U, 40U, 44U, 47U};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_16_0)
{
    constexpr std::array subject{25U, 36U, 37U, 40U, 44U, 47U, 51U, 54U,
                                 67U, 69U, 70U, 77U, 79U, 81U, 83U, 89U};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_20_0)
{
    constexpr std::array subject{25U, 36U, 37U, 40U, 44U, 47U, 51U,
                                 54U, 67U, 69U, 70U, 77U, 79U, 81U,
                                 83U, 89U, 93U, 95U, 98U, 100U};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests

// NOLINTEND(readability-implicit-bool-conversion)
// NOLINTEND(readability-magic-numbers)
