
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/cncr/math_supplement.hpp>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>

namespace dplx::dp
{

template <cncr::integer T>
class codec<T>
{
public:
    static auto encode(emit_context const &emit, T value) noexcept
            -> result<void>;
};

extern template class codec<signed char>;
extern template class codec<unsigned char>;
extern template class codec<short>;
extern template class codec<unsigned short>;
extern template class codec<int>;
extern template class codec<unsigned>;
extern template class codec<long>;
extern template class codec<unsigned long>;
extern template class codec<long long>;
extern template class codec<unsigned long long>;

} // namespace dplx::dp
