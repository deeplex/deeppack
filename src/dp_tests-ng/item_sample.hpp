
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

#include "dplx/dp/type_code.hpp"

namespace dp_tests
{

template <typename T>
struct item_sample
{
    T value;
    unsigned encoded_length;
    std::array<std::uint8_t, dplx::dp::detail::var_uint_max_size> encoded;

    template <typename U>
    [[nodiscard]] constexpr auto as() const noexcept -> item_sample<U>
    {
        return {static_cast<U>(value), encoded_length, encoded};
    }

    [[nodiscard]] auto encoded_bytes() const -> std::span<std::byte const>
    {
        return as_bytes(std::span(encoded))
                .first(std::min<std::size_t>(encoded_length, encoded.size()));
    }

    friend inline auto operator<<(std::ostream &os, item_sample const &sample)
            -> std::ostream &
    {
        fmt::print(os, "{{item_value: <please specialize me>, {}, 0x{:02x}}}",
                   sample.encoded_length,
                   fmt::join(sample.encoded_bytes(), "'"));
        return os;
    }
    friend inline auto operator<<(std::ostream &os, item_sample const &sample)
            -> std::ostream &requires(std::integral<T>)
    {
        fmt::print(os, "{{item_value: {:#x}, {}, 0x{:02x}}}", sample.value,
                   sample.encoded_length,
                   fmt::join(sample.encoded_bytes(), "'"));
        return os;
    }
    friend inline auto operator<<(std::ostream &os, item_sample const &sample)
            -> std::ostream &requires(std::floating_point<T>)
    {
        fmt::print(os, "{{item_value: {}, {}, 0x{:02x}}}", sample.value,
                   sample.encoded_length,
                   fmt::join(sample.encoded_bytes(), "'"));
        return os;
    }
};

} // namespace dp_tests
