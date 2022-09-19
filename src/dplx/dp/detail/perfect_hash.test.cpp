
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
    constexpr std::array subject{0u};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_02_0)
{
    constexpr std::array subject{0u, 1u};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_02_1)
{
    constexpr std::array subject{2u, 4u};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_03_0)
{
    constexpr std::array subject{2u, 3u, 4u};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_03_1)
{
    constexpr std::array subject{2u, 4u, 6u};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_06_0)
{
    constexpr std::array subject{25u, 36u, 37u, 40u, 44u, 46u};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_06_1)
{
    constexpr std::array subject{25u, 36u, 37u, 40u, 44u, 47u};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_16_0)
{
    constexpr std::array subject{25u, 36u, 37u, 40u, 44u, 47u, 51u, 54u,
                                 67u, 69u, 70u, 77u, 79u, 81u, 83u, 89u};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_CASE(h_20_0)
{
    constexpr std::array subject{25u, 36u, 37u, 40u, 44u, 47u, 51u,
                                 54u, 67u, 69u, 70u, 77u, 79u, 81u,
                                 83u, 89u, 93u, 95u, 98u, 100u};
    static_assert(!failing_hash(subject));
    BOOST_TEST(!failing_hash(subject));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
