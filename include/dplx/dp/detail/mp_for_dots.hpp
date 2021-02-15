
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

template <typename T>
struct mp_for_dots_impl;

template <std::size_t... Is>
struct mp_for_dots_impl<std::index_sequence<Is...>>
{
    template <class F>
    static constexpr auto invoke(F &&f) -> result<void>
    {
        result<void> rx = success();

        [[maybe_unused]] bool failed
                = (...
                   || detail::try_extract_failure(
                           static_cast<F &&>(f)(mp_size_t<Is>{}), rx));

        return rx;
    }
};

template <std::size_t N, typename F>
constexpr auto mp_for_dots(F &&f) -> result<void>
{
    return mp_for_dots_impl<std::make_index_sequence<N>>::invoke(
            static_cast<F &&>(f));
}

} // namespace dplx::dp::detail
