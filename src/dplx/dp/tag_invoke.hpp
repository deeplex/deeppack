
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <type_traits>

namespace dplx::dp
{

template <auto &Tag>
using tag_t = std::decay_t<decltype(Tag)>;

namespace detail::cpo_impl
{

struct tag_invoke_fn
{
    template <typename T, typename... TArgs>
        requires requires(T policy, TArgs &&...args)
        {
            tag_invoke(static_cast<T &&>(policy),
                       static_cast<TArgs &&>(args)...);
        }
    constexpr auto operator()(T policy, TArgs &&...args) const
            noexcept(noexcept(tag_invoke(static_cast<T &&>(policy),
                                         static_cast<TArgs &&>(args)...)))
                    -> decltype(tag_invoke(static_cast<T &&>(policy),
                                           static_cast<TArgs &&>(args)...))
    {
        return tag_invoke(static_cast<T &&>(policy),
                          static_cast<TArgs &&>(args)...);
    }
};

void tag_invoke() = delete;

} // namespace detail::cpo_impl

namespace cpo
{

inline constexpr detail::cpo_impl::tag_invoke_fn tag_invoke = {};

}

template <typename Tag, typename... TArgs>
concept tag_invocable
        = std::invocable<detail::cpo_impl::tag_invoke_fn, Tag, TArgs...>;

template <typename Tag, typename... TArgs>
concept nothrow_tag_invocable = tag_invocable<Tag, TArgs...> && std::
        is_nothrow_invocable_v<detail::cpo_impl::tag_invoke_fn, Tag, TArgs...>;

template <typename Tag, typename... TArgs>
using tag_invoke_result
        = std::invoke_result<detail::cpo_impl::tag_invoke_fn, Tag, TArgs...>;

template <typename Tag, typename... TArgs>
using tag_invoke_result_t
        = std::invoke_result_t<detail::cpo_impl::tag_invoke_fn, Tag, TArgs...>;

} // namespace dplx::dp
