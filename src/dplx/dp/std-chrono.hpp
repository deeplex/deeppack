
// Copyright Henrik Steffen Gaßmann 2020, 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <chrono>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/emit_core.hpp>

namespace dplx::dp
{

template <typename Rep, typename Period>
class codec<std::chrono::duration<Rep, Period>>
{
public:
    static auto encode(emit_context const &ctx,
                       std::chrono::duration<Rep, Period> value) noexcept
            -> result<void>
    {
        return dp::emit_integer(ctx, value.count());
    }
};

extern template class codec<std::chrono::nanoseconds>;
extern template class codec<std::chrono::microseconds>;
extern template class codec<std::chrono::milliseconds>;
extern template class codec<std::chrono::seconds>;
extern template class codec<std::chrono::minutes>;
extern template class codec<std::chrono::hours>;
extern template class codec<std::chrono::days>;
extern template class codec<std::chrono::weeks>;
extern template class codec<std::chrono::months>;
extern template class codec<std::chrono::years>;

} // namespace dplx::dp
