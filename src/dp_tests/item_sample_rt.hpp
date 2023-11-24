
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <span>
#include <vector>

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include "test_utils.hpp"

namespace dp_tests
{

template <typename T>
struct item_sample_rt
{
    std::string name;
    std::vector<std::byte> encoded;
    T value;

    template <typename U>
    [[nodiscard]] constexpr auto as() const noexcept -> item_sample_rt<U>
    {
        return {static_cast<U>(value), encoded};
    }

    [[nodiscard]] auto encoded_bytes() const -> std::span<std::byte const>
    {
        return std::span(encoded);
    }

    friend inline auto operator<<(std::ostream &os,
                                  item_sample_rt const &sample)
            -> std::ostream &requires(!detail::is_fmt_formattable<T const &>) {
                fmt::print(
                        os,
                        "{{item_value[{}]: <please specialize me>, 0x{:02x}}}",
                        sample.name, fmt::join(sample.encoded, "'"));
                return os;
            } friend inline auto
            operator<<(std::ostream &os, item_sample_rt const &sample)
                    -> std::ostream &
    {
        fmt::print(os, "{{item_value[{}]: {}, 0x{:02x}}}", sample.name,
                   sample.value, fmt::join(sample.encoded, "'"));
        return os;
    }
    friend inline auto operator<<(std::ostream &os,
                                  item_sample_rt const &sample)
            -> std::ostream &
        requires std::integral<T>
    {
        fmt::print(os, "{{item_value[{}]: {:#x}, 0x{:02x}}}", sample.name,
                   sample.value, fmt::join(sample.encoded, "'"));
        return os;
    }
    friend inline auto operator<<(std::ostream &os,
                                  item_sample_rt const &sample)
            -> std::ostream &
        requires std::floating_point<T>
    {
        fmt::print(os, "{{item_value[{}]: {}, 0x{:02x}}}", sample.name,
                   sample.value, fmt::join(sample.encoded, "'"));
        return os;
    }
};

} // namespace dp_tests
