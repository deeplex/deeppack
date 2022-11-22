
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

// defines the following encoder specializations
//  * null_type
//  * bool
//  * integer
//  * iec559 floating point

#include <dplx/cncr/math_supplement.hpp>
#include <dplx/predef/compiler.h>

#include <dplx/dp/detail/workaround.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>

#if DPLX_DP_WORKAROUND_TESTED_AT(DPLX_COMP_CLANG, 15, 0, 0)
#include <dplx/dp/items/emit.hpp>
#endif

namespace dplx::dp
{

template <>
class codec<null_type>
{
public:
    static auto encode(emit_context const &ctx, null_type) noexcept
            -> result<void>;
};

template <cncr::integer T>
class codec<T>
{
public:
    // https://github.com/llvm/llvm-project/issues/49620
    // or
    // https://github.com/llvm/llvm-project/issues/58124
#if DPLX_DP_WORKAROUND_TESTED_AT(DPLX_COMP_CLANG, 15, 0, 0)
    static auto encode(emit_context const &ctx, T value) noexcept
            -> result<void>
    {
        return dp::emit_integer(ctx, value);
    }
#else
    static auto encode(emit_context const &ctx, T value) noexcept
            -> result<void>;
#endif
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

template <>
class codec<bool>
{
public:
    static auto encode(emit_context const &ctx, bool value) noexcept
            -> result<void>;
};

template <>
class codec<float>
{
public:
    static auto encode(emit_context const &ctx, float value) noexcept
            -> result<void>;
};

template <>
class codec<double>
{
public:
    static auto encode(emit_context const &ctx, double value) noexcept
            -> result<void>;
};

} // namespace dplx::dp
