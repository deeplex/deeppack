
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>

#include <algorithm>

#include <boost/predef/other/workaround.h>

#include <dplx/dp/decoder/core.hpp>
#include <dplx/dp/detail/bit.hpp>
#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/map_pair.hpp>

namespace dplx::dp::detail
{

constexpr auto is_prime(std::uint64_t const i) noexcept -> bool
{
    // if (i == 2 || i == 3)
    // {
    //     return true;
    // }
    if ((i % 2) == 0 || (i % 3) == 0)
    {
        return false;
    }

    // we ultimately want to check whether i is divisible by a prime
    // therefore we use the following implication to reduce the amount
    // of _redundant_ divisibility checks
    // p is prime ∧ p > 3 => ∃k ∈ N. p = (k*6) ± 1
    for (std::uint64_t divisor = 6;
         divisor * divisor - 2 * divisor + 1 <= i; // <=> (divisor - 1)^2 <= i
         divisor += 6)
    {
        if (i % (divisor - 1) == 0 || i % (divisor + 1) == 0)
        {
            return false;
        }
    }
    return true;
}

constexpr auto next_prime(std::uint64_t const i) noexcept -> std::uint64_t
{
    // linear search
    // 53 > 255 / 5
#define DPLX_XPRIME(p) else if (i <= (p)) return p
    if (i <= 2)
        return 2;
    DPLX_XPRIME(3);
    DPLX_XPRIME(5);
    DPLX_XPRIME(7);
    DPLX_XPRIME(11);
    DPLX_XPRIME(13);
    DPLX_XPRIME(17);
    DPLX_XPRIME(19);
    DPLX_XPRIME(23);
    DPLX_XPRIME(29);
    DPLX_XPRIME(31);
    DPLX_XPRIME(37);
    DPLX_XPRIME(41);
    DPLX_XPRIME(43);
    DPLX_XPRIME(47);
    DPLX_XPRIME(53);
#undef DPLX_XPRIME
    for (auto j = i | 1u; true; j += 2)
    {
        if (is_prime(j))
        {
            return j;
        }
    }
}

// maps N keys of type T uniquely to the integer interval [0, N)
template <typename T, std::size_t N, typename KeyHash>
struct perfect_hasher
{
    static constexpr KeyHash key_hash{};
    static constexpr std::uint64_t initial_seed = 0x8000'0000'0000'0000;
    static constexpr std::size_t remap_size = next_prime(N / 5);
    static_assert(remap_size >= (N / 5));

    std::array<std::uint64_t, remap_size> remap;
    std::array<std::size_t, N> values;

public:
    static constexpr auto invalid_value = ~static_cast<std::size_t>(0);

    constexpr explicit perfect_hasher(std::array<T, N> const &keys) noexcept
        : remap{}
        , values{}
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            values[i] = invalid_value;
        }

        using pattern_type = std::array<std::size_t, N + 1>;
        std::array<pattern_type, remap_size> remapPatterns{};

        for (std::size_t i = 0; i < N; ++i)
        {
            auto const &key = keys[i];
            auto const flhash
                    = static_cast<std::size_t>(key_hash(key) % remap_size);
            auto &pattern = remapPatterns[flhash];
            auto const pos = pattern.back()++;
            pattern[pos] = i;
        }

        std::sort(remapPatterns.data(), remapPatterns.data() + remap_size,
                  [](pattern_type const &l, pattern_type const &r)
                  { return l.back() > r.back(); });

        std::size_t i = 0;
        for (; i < remap_size && remapPatterns[i].back() > 1; ++i)
        {
            auto const &pattern = remapPatterns[i];

            auto seed = initial_seed;
            std::array<std::size_t, N> slots;

            for (std::size_t j = 0; j < pattern.back();)
            {
                auto const &key = keys[pattern[j]];
                auto const hash = key_hash(key, seed);

                auto const slot = hash % values.size();

                auto const slotPreviouslyAssigned
                        = values[slot] != invalid_value;

                if (auto const slotsEnd = slots.data() + j;
                    slotPreviouslyAssigned
                    || std::find(slots.data(), slotsEnd, slot) != slotsEnd)
                {
                    // current seed would generate a collision => reroll
                    j = 0;
                    seed += 1;
                    seed ^= (hash & 0x7fff'ffff'0000'0000);
                }
                else
                {
                    slots[j] = slot;
                    j += 1;
                }
            }

            auto const &key = keys[pattern[0]];
            auto const flhash
                    = static_cast<std::size_t>(key_hash(key) % remap_size);
            remap[flhash] = seed;

            for (std::size_t j = 0; j < pattern.back(); ++j)
            {
                values[slots[j]] = pattern[j];
            }
        }

        std::array<std::uint64_t, N + 1> freeSlots{};
        for (std::size_t j = 0; j < N; ++j)
        {
            if (values[j] == invalid_value)
            {
                auto const pos = freeSlots.back()++;
                freeSlots[pos] = j;
            }
        }

        for (; i < remap_size && remapPatterns[i].back() != 0; ++i)
        {
            auto const &key = keys[remapPatterns[i][0]];
            auto const slot = freeSlots[--freeSlots.back()];
            remap[key_hash(key) % remap_size] = slot;

#if BOOST_PREDEF_WORKAROUND(BOOST_COMP_GNUC, <=, 10, 1, 0)
            // gcc has a problem with the defaulted <=> over structs containing
            // arrays
            values[slot] = static_cast<std::size_t>(
                    std::find(keys.data(), keys.data() + keys.size(), key)
                    - keys.data());
#else
            values[slot] = static_cast<std::size_t>(
                    std::lower_bound(keys.data(), keys.data() + keys.size(),
                                     key)
                    - keys.data());
#endif
        }
    }

    template <typename TLike>
    constexpr auto operator()(TLike &&key) const -> std::size_t
    {
        auto const remapped = remap[key_hash(key) % remap_size];
        if ((remapped & 0x8000'0000'0000'0000) == 0)
        {
            return values[remapped];
        }
        else
        {
            auto const rehashed = key_hash(key, remapped);
            return values[rehashed % values.size()];
        }
    }
};

} // namespace dplx::dp::detail
