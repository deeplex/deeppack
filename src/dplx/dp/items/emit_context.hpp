
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/dp/fwd.hpp>

namespace dplx::dp
{

struct emit_context
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    output_buffer &out;
};

} // namespace dplx::dp
