
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/core.hpp"

#include <dplx/dp/items/emit.hpp>

namespace dplx::dp
{

auto codec<null_type>::encode(emit_context const &ctx, null_type) noexcept
        -> result<void>
{
    return dp::emit_null(ctx);
}

#if DPLX_DP_WORKAROUND_TESTED_AT(DPLX_COMP_CLANG, 15, 0, 0)
#else

template <cncr::integer T>
auto codec<T>::encode(emit_context const &ctx, T value) noexcept -> result<void>
{
    return dp::emit_integer(ctx, value);
}

#endif

template class codec<signed char>;
template class codec<unsigned char>;
template class codec<short>;
template class codec<unsigned short>;
template class codec<int>;
template class codec<unsigned>;
template class codec<long>;
template class codec<unsigned long>;
template class codec<long long>;
template class codec<unsigned long long>;

auto codec<bool>::encode(emit_context const &ctx, bool value) noexcept
        -> result<void>
{
    return dp::emit_boolean(ctx, value);
}

auto codec<float>::encode(emit_context const &ctx, float value) noexcept
        -> result<void>
{
    return dp::emit_float_single(ctx, value);
}

auto codec<double>::encode(emit_context const &ctx, double value) noexcept
        -> result<void>
{
    return dp::emit_float_double(ctx, value);
}

} // namespace dplx::dp
