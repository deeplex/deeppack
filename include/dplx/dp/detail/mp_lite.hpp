
// Copyright Peter Dimov 2015-2017.
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <type_traits>

namespace dplx::dp::detail
{

template <typename... Ts>
struct mp_list
{
};

template <typename T, template <typename...> typename U>
struct mp_rename
{
};
template <template <typename...> typename T,
          template <typename...>
          typename U,
          typename... Ts>
struct mp_rename<T<Ts...>, U>
{
    using type = U<Ts...>;
};
template <typename T, template <typename...> typename U>
using mp_rename_t = typename mp_rename<T, U>::type;

template <template <typename> typename Fn, typename T>
struct mp_transform
{
};
template <template <typename> typename Fn,
          template <typename...>
          typename L,
          typename... TArgs>
struct mp_transform<Fn, L<TArgs...>>
{
    using type = L<Fn<TArgs>...>;
};
template <template <typename> typename Fn, typename T>
using mp_transform_t = typename mp_transform<Fn, T>::type;

template <std::size_t I>
using mp_size_t = std::integral_constant<std::size_t, I>;

} // namespace dplx::dp::detail
