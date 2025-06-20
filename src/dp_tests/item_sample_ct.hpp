
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <span>

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

#include "dplx/dp/items/type_code.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

template <typename T, std::size_t MaxSize = dp::detail::var_uint_max_size>
struct item_sample_ct
{
    T value;
    unsigned encoded_length;
    std::array<std::uint8_t, MaxSize> encoded;

    template <typename U>
    [[nodiscard]] constexpr auto as() const noexcept -> item_sample_ct<U>
    {
        return {static_cast<U>(value), encoded_length, encoded};
    }

    [[nodiscard]] auto encoded_bytes() const -> std::span<std::byte const>
    {
        return as_bytes(std::span(encoded))
                .first(std::min<std::size_t>(encoded_length, encoded.size()));
    }

    friend inline auto operator<<(std::ostream &os,
                                  item_sample_ct const &sample)
            -> std::ostream &
        requires(!detail::is_fmt_formattable<T const &>)
    {
        fmt::print(os, "{{item_value: <please specialize me>, {}, 0x{:02x}}}",
                   sample.encoded_length,
                   fmt::join(sample.encoded_bytes(), "'"));
        return os;
    }
    friend inline auto operator<<(std::ostream &os,
                                  item_sample_ct const &sample)
            -> std::ostream &
    {
        fmt::print(os, "{{item_value: {}, {}, 0x{:02x}}}", sample.value,
                   sample.encoded_length,
                   fmt::join(sample.encoded_bytes(), "'"));
        return os;
    }
    friend inline auto operator<<(std::ostream &os,
                                  item_sample_ct const &sample)
            -> std::ostream &
        requires std::integral<T>
    {
        fmt::print(os, "{{item_value: {:#x}, {}, 0x{:02x}}}", sample.value,
                   sample.encoded_length,
                   fmt::join(sample.encoded_bytes(), "'"));
        return os;
    }
    friend inline auto operator<<(std::ostream &os,
                                  item_sample_ct const &sample)
            -> std::ostream &
        requires std::floating_point<T>
    {
        fmt::print(os, "{{item_value: {}, {}, 0x{:02x}}}", sample.value,
                   sample.encoded_length,
                   fmt::join(sample.encoded_bytes(), "'"));
        return os;
    }
};

} // namespace dp_tests
