
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <algorithm>
#include <cstdint>

#include <dplx/predef/compiler.h>

#include <dplx/dp/decoder/core.hpp>
#include <dplx/dp/detail/bit.hpp>
#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/detail/workaround.hpp>
#include <dplx/dp/map_pair.hpp>

namespace dplx::dp::detail
{

constexpr auto is_prime(std::uint64_t const i) noexcept -> bool
{
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)

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
    for (std::uint64_t divisor = 6U;
         divisor * divisor - 2U * divisor + 1U <= i; // <=> (divisor - 1)^2 <= i
         divisor += 6U)
    {
        if (i % (divisor - 1U) == 0U || i % (divisor + 1U) == 0U)
        {
            return false;
        }
    }
    return true;
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}

constexpr auto next_prime(std::uint64_t const i) noexcept -> std::uint64_t
{
    // linear search
    // 53 > 255 / 5
    // NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define DPLX_XPRIME(p)                                                         \
    if (i <= (p))                                                              \
    {                                                                          \
        return p;                                                              \
    }
    DPLX_XPRIME(2U)
    DPLX_XPRIME(3U)
    DPLX_XPRIME(5U)
    DPLX_XPRIME(7U)
    DPLX_XPRIME(11U)
    DPLX_XPRIME(13U)
    DPLX_XPRIME(17U)
    DPLX_XPRIME(19U)
    DPLX_XPRIME(23U)
    DPLX_XPRIME(29U)
    DPLX_XPRIME(31U)
    DPLX_XPRIME(37U)
    DPLX_XPRIME(41U)
    DPLX_XPRIME(43U)
    DPLX_XPRIME(47U)
    DPLX_XPRIME(53U)
#undef DPLX_XPRIME
    // NOLINTEND(cppcoreguidelines-macro-usage)

    for (auto j = i | 1U; true; j += 2)
    {
        if (is_prime(j))
        {
            return j;
        }
    }
}

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)

// maps N keys of type T uniquely to the integer interval [0, N)
template <typename T, std::size_t N, typename KeyHash>
struct perfect_hasher
{
    static constexpr KeyHash key_hash{};
    static constexpr std::uint64_t initial_seed = 0x8000'0000'0000'0000U;
    static constexpr std::size_t remap_size = detail::next_prime(N / 5U);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    static_assert(remap_size >= (N / 5U));

    std::array<std::uint64_t, remap_size> remap;
    std::array<std::size_t, N> values;

public:
    static constexpr auto invalid_value = ~static_cast<std::size_t>(0U);

    constexpr explicit perfect_hasher(std::array<T, N> const &keys) noexcept
        : remap{}
        , values{}
    {
        for (std::size_t i = 0U; i < N; ++i)
        {
            values[i] = invalid_value;
        }

        // last/.back() is the number of keys in a pattern
        using pattern_type = std::array<std::size_t, N + 1U>;
        std::array<pattern_type, remap_size> remapPatterns{};

        for (std::size_t i = 0U; i < N; ++i)
        {
            auto const &key = keys[i];
            auto const flhash
                    = static_cast<std::size_t>(key_hash(key) % remap_size);
            auto &pattern = remapPatterns[flhash];
            auto const pos = pattern.back()++;
            pattern[pos] = i;
        }

        // sort patterns by the number of keys in each one
        std::sort(remapPatterns.data(), remapPatterns.data() + remap_size,
                  [](pattern_type const &l, pattern_type const &r)
                  { return l.back() > r.back(); });

        std::size_t i = 0U;
        // find a seed for every pattern with more than one key which maps these
        // keys to unused slots
        for (; i < remap_size && remapPatterns[i].back() > 1U; ++i)
        {
            auto const &pattern = remapPatterns[i];

            auto seed = initial_seed;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
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
                    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
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

        // collect the slots not used by patterns with many keys
        std::array<std::uint64_t, N + 1> freeSlots{};
        for (std::size_t j = 0; j < N; ++j)
        {
            if (values[j] == invalid_value)
            {
                auto const pos = freeSlots.back()++;
                freeSlots[pos] = j;
            }
        }

        // assign the free slots to the remaing pattern with exactly one key
        for (; i < remap_size && remapPatterns[i].back() != 0; ++i)
        {
            auto const &key = keys[remapPatterns[i][0]];
            auto const slot = freeSlots[--freeSlots.back()];
            remap[key_hash(key) % remap_size] = slot;

#if DPLX_DP_WORKAROUND(DPLX_COMP_GNUC, <=, 10, 1, 0)
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
        std::uint64_t const remapped = remap[key_hash(key) % remap_size];
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        if ((remapped & 0x8000'0000'0000'0000U) == 0U)
        {
            return values[remapped];
        }
        auto const rehashed = key_hash(key, remapped);
        return values[rehashed % N];
    }
};

// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

} // namespace dplx::dp::detail
