
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <dplx/dp/detail/mp_lite.hpp>
#include <dplx/dp/disappointment.hpp>

namespace dplx::dp::detail
{

template <std::size_t N>
struct mp_for_dots_impl;

template <std::size_t N, typename F>
constexpr auto mp_for_dots(F &&f) -> result<void>
{
    static_assert(N > 0);
    return mp_for_dots_impl<N>::template invoke<0>(f);
}

template <>
struct mp_for_dots_impl<0>
{
};

template <>
struct mp_for_dots_impl<1>
{
    template <std::size_t I, class F>
    static constexpr auto invoke(F &&f) -> result<void>
    {
        DPLX_TRY(f(mp_size_t<I + 0>{}));

        return success();
    }
};

template <>
struct mp_for_dots_impl<2>
{
    template <std::size_t I, class F>
    static constexpr auto invoke(F &&f) -> result<void>
    {
        DPLX_TRY(f(mp_size_t<I + 0>{}));
        DPLX_TRY(f(mp_size_t<I + 1>{}));

        return success();
    }
};

template <>
struct mp_for_dots_impl<3>
{
    template <std::size_t I, class F>
    static constexpr auto invoke(F &&f) -> result<void>
    {
        DPLX_TRY(f(mp_size_t<I + 0>{}));
        DPLX_TRY(f(mp_size_t<I + 1>{}));
        DPLX_TRY(f(mp_size_t<I + 2>{}));

        return success();
    }
};

template <>
struct mp_for_dots_impl<4>
{
    template <std::size_t I, class F>
    static constexpr auto invoke(F &&f) -> result<void>
    {
        DPLX_TRY(f(mp_size_t<I + 0>{}));
        DPLX_TRY(f(mp_size_t<I + 1>{}));
        DPLX_TRY(f(mp_size_t<I + 2>{}));
        DPLX_TRY(f(mp_size_t<I + 3>{}));

        return success();
    }
};

template <>
struct mp_for_dots_impl<5>
{
    template <std::size_t I, class F>
    static constexpr auto invoke(F &&f) -> result<void>
    {
        DPLX_TRY(f(mp_size_t<I + 0>{}));
        DPLX_TRY(f(mp_size_t<I + 1>{}));
        DPLX_TRY(f(mp_size_t<I + 2>{}));
        DPLX_TRY(f(mp_size_t<I + 3>{}));
        DPLX_TRY(f(mp_size_t<I + 4>{}));

        return success();
    }
};

template <>
struct mp_for_dots_impl<6>
{
    template <std::size_t I, class F>
    static constexpr auto invoke(F &&f) -> result<void>
    {
        DPLX_TRY(f(mp_size_t<I + 0>{}));
        DPLX_TRY(f(mp_size_t<I + 1>{}));
        DPLX_TRY(f(mp_size_t<I + 2>{}));
        DPLX_TRY(f(mp_size_t<I + 3>{}));
        DPLX_TRY(f(mp_size_t<I + 4>{}));
        DPLX_TRY(f(mp_size_t<I + 5>{}));

        return success();
    }
};

template <>
struct mp_for_dots_impl<7>
{
    template <std::size_t I, class F>
    static constexpr auto invoke(F &&f) -> result<void>
    {
        DPLX_TRY(f(mp_size_t<I + 0>{}));
        DPLX_TRY(f(mp_size_t<I + 1>{}));
        DPLX_TRY(f(mp_size_t<I + 2>{}));
        DPLX_TRY(f(mp_size_t<I + 3>{}));
        DPLX_TRY(f(mp_size_t<I + 4>{}));
        DPLX_TRY(f(mp_size_t<I + 5>{}));
        DPLX_TRY(f(mp_size_t<I + 6>{}));

        return success();
    }
};

template <>
struct mp_for_dots_impl<8>
{
    template <std::size_t I, class F>
    static constexpr auto invoke(F &&f) -> result<void>
    {
        DPLX_TRY(f(mp_size_t<I + 0>{}));
        DPLX_TRY(f(mp_size_t<I + 1>{}));
        DPLX_TRY(f(mp_size_t<I + 2>{}));
        DPLX_TRY(f(mp_size_t<I + 3>{}));
        DPLX_TRY(f(mp_size_t<I + 4>{}));
        DPLX_TRY(f(mp_size_t<I + 5>{}));
        DPLX_TRY(f(mp_size_t<I + 6>{}));
        DPLX_TRY(f(mp_size_t<I + 7>{}));

        return success();
    }
};

template <std::size_t N> // > 8
struct mp_for_dots_impl
{
    template <std::size_t I, class F>
    static constexpr auto invoke(F &&f) -> result<void>
    {
        DPLX_TRY(f(mp_size_t<I + 0>{}));
        DPLX_TRY(f(mp_size_t<I + 1>{}));
        DPLX_TRY(f(mp_size_t<I + 2>{}));
        DPLX_TRY(f(mp_size_t<I + 3>{}));
        DPLX_TRY(f(mp_size_t<I + 4>{}));
        DPLX_TRY(f(mp_size_t<I + 5>{}));
        DPLX_TRY(f(mp_size_t<I + 6>{}));
        DPLX_TRY(f(mp_size_t<I + 7>{}));

        return mp_for_dots_impl<N - 8>::template invoke<I + 8>(f);
    }
};

} // namespace dplx::dp::detail
