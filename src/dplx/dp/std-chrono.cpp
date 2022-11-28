
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/std-chrono.hpp"

namespace dplx::dp
{

template class codec<std::chrono::nanoseconds>;
template class codec<std::chrono::microseconds>;
template class codec<std::chrono::milliseconds>;
template class codec<std::chrono::seconds>;
template class codec<std::chrono::minutes>;
template class codec<std::chrono::hours>;
template class codec<std::chrono::days>;
template class codec<std::chrono::weeks>;
template class codec<std::chrono::months>;
template class codec<std::chrono::years>;

} // namespace dplx::dp
