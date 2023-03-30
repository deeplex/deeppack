
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <memory_resource>

#include <dplx/dp/fwd.hpp>
#include <dplx/dp/state.hpp>

namespace dplx::dp
{

struct parse_context
{
    input_buffer &in;
    state_store states;
    link_store links;

    explicit parse_context(
            input_buffer &inStreamBuffer,
            std::pmr::polymorphic_allocator<std::byte> const &allocator
            = std::pmr::polymorphic_allocator<std::byte>{})
        : in(inStreamBuffer)
        , states(allocator)
        , links(allocator)
    {
    }

    [[nodiscard]] auto get_allocator() const noexcept
            -> std::pmr::polymorphic_allocator<std::byte>
    {
        return states.get_allocator();
    }
};

} // namespace dplx::dp
