
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/core.hpp"

#include <dplx/dp/items/emit.hpp>

namespace dplx::dp
{

template <cncr::integer T>
auto codec<T>::encode(emit_context const &ctx, T value) noexcept -> result<void>
{
    return emit_integer(ctx, value);
}

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

} // namespace dplx::dp
