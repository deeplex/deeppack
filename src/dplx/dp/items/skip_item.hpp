
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>

namespace dplx::dp
{

auto skip_item(parse_context &ctx) noexcept -> result<void>;

} // namespace dplx::dp
