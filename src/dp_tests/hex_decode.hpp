
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace dp_tests
{

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)

constexpr auto is_hex_digit(char const digit) noexcept -> bool
{
    unsigned const udigit = static_cast<unsigned char>(digit);
    return (udigit - '0' <= 9U) || ((udigit | 0x20U) - 'a' <= 5U);
}

constexpr auto value_of_hex_digit(char const digit) noexcept -> std::uint8_t
{
    return static_cast<std::uint8_t>(
            (static_cast<unsigned>(digit) & 0xfU)
            + ((static_cast<unsigned>(digit) >> 6U) * 9U));
}

constexpr auto hex_decode(std::uint8_t *const decoded,
                          char const *const hex,
                          std::size_t const hexSize) noexcept -> std::uint8_t *
{
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    auto *outIt = decoded;
    auto const *srcIt = hex;
    auto const *const end = srcIt + hexSize;

    while (srcIt != end)
    {
        std::uint8_t const hi = value_of_hex_digit(*(srcIt++));
        std::uint8_t const lo = value_of_hex_digit(*(srcIt++));
        *(outIt++) = static_cast<std::uint8_t>((hi << 4) | lo);
    }
    return outIt;

    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

inline auto hex_decode(char const *const hex,
                       std::size_t const hexSize) noexcept
        -> std::vector<std::byte>
{
    std::vector<std::byte> memory(hexSize / 2U);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    dp_tests::hex_decode(reinterpret_cast<std::uint8_t *>(memory.data()), hex,
                         hexSize);
    return memory;
}

inline auto hex_decode(std::string_view const hex) noexcept
        -> std::vector<std::byte>
{
    return dp_tests::hex_decode(hex.data(), hex.size());
}

} // namespace dp_tests
