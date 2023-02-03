
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <filesystem>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>

namespace dplx::dp
{

template <>
class codec<std::filesystem::path>
{
public:
    static auto size_of(emit_context &ctx,
                        std::filesystem::path const &path) noexcept
            -> std::uint64_t;
    static auto encode(emit_context &ctx,
                       std::filesystem::path const &path) noexcept
            -> result<void>;
    static auto decode(parse_context &ctx, std::filesystem::path &path) noexcept
            -> result<void>;
};

} // namespace dplx::dp
