
// Copyright 2023 Henrik Steffen Gaßmann
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/cncr/uuid.hpp>

#include <dplx/dp/fwd.hpp>

namespace dplx::dp
{

template <>
class codec<cncr::uuid>
{
public:
    static auto decode(parse_context &ctx, cncr::uuid &value) noexcept
            -> result<void>;
    static auto encode(emit_context &ctx, cncr::uuid value) noexcept
            -> result<void>;
    static auto size_of(emit_context &ctx, cncr::uuid value) noexcept
            -> std::uint64_t;
};

} // namespace dplx::dp
