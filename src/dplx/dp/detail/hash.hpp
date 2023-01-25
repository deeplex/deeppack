
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstdint>

#include <dplx/dp/detail/bit.hpp>
#include <dplx/dp/detail/type_utils.hpp>

namespace dplx::dp::detail
{

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)

template <typename T>
constexpr auto fnvx_hash(T const *const data,
                         std::size_t const size,
                         std::uint64_t const seed) noexcept -> std::uint64_t
{
    std::uint64_t state = {0x243f'6a88'85a3'08d3 ^ seed};
    for (std::size_t i = 0; i < size; ++i)
    {
        state *= 0x00000100000001B3;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        state ^= static_cast<std::uint8_t>(data[i]);
    }

    // rrmxmx
    std::uint64_t const r1 = detail::rotl(state, 49);
    std::uint64_t const r2 = detail::rotl(state, 24);
    std::uint64_t const rx = r1 ^ r2;

    std::uint64_t const m1 = rx * 0x9fb2'1c65'1e98'df25;
    std::uint64_t const x1 = m1 ^ ((m1 >> 35) + size);

    std::uint64_t const m2 = x1 * 0x9fb2'1c65'1e98'df25;
    std::uint64_t const x2 = m2 ^ (m2 >> 28);

    return x2;
}

constexpr auto xxhash3_fixed_impl(std::uint64_t const val,
                                  std::uint64_t const seed,
                                  unsigned int length) noexcept -> std::uint64_t
{
    constexpr std::uint64_t iv0 = 0x1cad'212c'81f7'017c;
    constexpr std::uint64_t iv1 = 0xdb97'9083'e96d'd4de;
    constexpr std::uint64_t ivx = iv0 ^ iv1;

    std::uint64_t const mixedSeed
            = seed
            ^ (static_cast<std::uint64_t>(
                       detail::byte_swap_u32(static_cast<std::uint32_t>(seed)))
               << 32);

    std::uint64_t const xorpad = ivx - mixedSeed;
    std::uint64_t const spread = val ^ xorpad;

    // rrmxmx
    std::uint64_t const r1 = detail::rotl(spread, 49);
    std::uint64_t const r2 = detail::rotl(spread, 24);
    std::uint64_t const rx = r1 ^ r2;

    std::uint64_t const m1 = rx * 0x9fb2'1c65'1e98'df25;
    std::uint64_t const x1 = m1 ^ ((m1 >> 35) + length);

    std::uint64_t const m2 = x1 * 0x9fb2'1c65'1e98'df25;
    std::uint64_t const x2 = m2 ^ (m2 >> 28);

    return x2;
}

constexpr auto xxhash3_ui32(std::uint32_t const value,
                            std::uint64_t const seed) noexcept -> std::uint64_t
{
    std::uint64_t const stretched
            = value | (static_cast<std::uint64_t>(value) << 32);
    return xxhash3_fixed_impl(stretched, seed, 4U);
}

constexpr auto xxhash3_ui64(std::uint64_t const value,
                            std::uint64_t const seed) noexcept -> std::uint64_t
{
    std::uint64_t const swapped = (value << 32) | (value >> 32);
    return xxhash3_fixed_impl(swapped, seed, 8U);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

template <cncr::integer T>
    requires(sizeof(T) <= sizeof(std::uint64_t))
constexpr auto xxhash3(T const value, std::uint64_t const seed) noexcept
        -> std::uint64_t
{
    if constexpr (sizeof(T) <= sizeof(std::uint32_t))
    {
        return xxhash3_ui32(static_cast<std::uint32_t>(value), seed);
    }
    else if constexpr (sizeof(T) <= sizeof(std::uint64_t))
    {
        return xxhash3_ui64(static_cast<std::uint64_t>(value), seed);
    }
}

} // namespace dplx::dp::detail
