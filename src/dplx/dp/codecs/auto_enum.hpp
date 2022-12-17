
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <type_traits>

#include <dplx/dp/api.hpp>
#include <dplx/dp/codecs/core.hpp>
#include <dplx/dp/concepts.hpp>
#include <dplx/dp/fwd.hpp>

namespace dplx::dp
{

template <codable_enum T>
class codec<T>
{
public:
    static auto size_of(emit_context const &ctx, T value) noexcept
            -> std::uint64_t
    {
        return dp::encoded_size_of(
                ctx, static_cast<std::underlying_type_t<T>>(value));
    }
    static auto encode(emit_context const &ctx, T value) noexcept
            -> result<void>
    {
        return dp::encode(ctx, static_cast<std::underlying_type_t<T>>(value));
    }
};

} // namespace dplx::dp
