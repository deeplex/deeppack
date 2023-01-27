
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cassert>
#include <cstdint>
#include <ranges>

#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/encoded_item_head_size.hpp>
#include <dplx/dp/items/type_code.hpp>

namespace dplx::dp
{

namespace detail
{

// clang-format off
template <typename Fn, typename R>
concept subitem_size_of
    = requires(Fn &&sizeOfFn,
               emit_context const ctx,
               std::ranges::range_reference_t<R const> v)
    {
        { static_cast<Fn &&>(sizeOfFn)(ctx, v) }
            -> std::same_as<std::uint64_t>;
    };
// clang-format on

template <typename R, typename SizeOfElementFn>
inline auto item_size_of_array_like(emit_context const &ctx,
                                    R const &vs,
                                    SizeOfElementFn &&sizeOfElement) noexcept
        -> std::uint64_t
{
    if constexpr (std::ranges::sized_range<R>)
    {
        detail::encodable_int auto const numElements = std::ranges::size(vs);
        // map and array item head sizes don't differ
        std::uint64_t size
                = dp::encoded_item_head_size<type_code::array>(numElements);

        for (auto &&v : vs)
        {
            size += static_cast<SizeOfElementFn &&>(sizeOfElement)(ctx, v);
        }
        return size;
    }
    else
    {
        auto it = std::ranges::begin(vs);
        auto const end = std::ranges::end(vs);
        detail::encodable_int auto const numElements
                = std::ranges::distance(it, end);

        // map and array item head sizes don't differ
        std::uint64_t size
                = dp::encoded_item_head_size<type_code::array>(numElements);

        for (; it != end; ++it)
        {
            size += static_cast<SizeOfElementFn &&>(sizeOfElement)(ctx, *it);
        }
        return size;
    }
}
template <typename R, typename SizeOfElementFn>
inline auto
item_size_of_indefinite_array_like(emit_context const &ctx,
                                   R const &vs,
                                   SizeOfElementFn &&sizeOfElement) noexcept
        -> std::uint64_t
{
    // begin and special break
    std::uint64_t size = 1U + 1U;

    for (auto &&v : vs)
    {
        size += static_cast<SizeOfElementFn &&>(sizeOfElement)(ctx, v);
    }
    return size;
}

} // namespace detail

template <std::ranges::input_range R, typename SizeOfElementFn>
    // clang-format off
    requires((std::ranges::forward_range<R> || std::ranges::sized_range<R>)
            && detail::subitem_size_of<std::remove_cvref_t<SizeOfElementFn>, R>)
// clang-format on
inline auto item_size_of_array(emit_context const &ctx,
                               R const &vs,
                               SizeOfElementFn &&sizeOfElement) noexcept
        -> std::uint64_t
{
    return detail::item_size_of_array_like(
            ctx, vs, static_cast<SizeOfElementFn &&>(sizeOfElement));
}
template <std::ranges::input_range R, typename SizeOfElementFn>
    requires(detail::subitem_size_of<std::remove_cvref_t<SizeOfElementFn>, R>)
inline auto
item_size_of_array_indefinite(emit_context const &ctx,
                              R const &vs,
                              SizeOfElementFn &&sizeOfElement) noexcept
        -> std::uint64_t
{
    return detail::item_size_of_indefinite_array_like(
            ctx, vs, static_cast<SizeOfElementFn &&>(sizeOfElement));
}

template <std::ranges::input_range R, typename SizeOfElementFn>
    // clang-format off
    requires((std::ranges::forward_range<R> || std::ranges::sized_range<R>)
            && detail::subitem_size_of<std::remove_cvref_t<SizeOfElementFn>, R>)
// clang-format on
inline auto item_size_of_map(emit_context const &ctx,
                             R const &vs,
                             SizeOfElementFn &&sizeOfElement) noexcept
        -> std::uint64_t
{
    return detail::item_size_of_array_like(
            ctx, vs, static_cast<SizeOfElementFn &&>(sizeOfElement));
}
template <std::ranges::input_range R, typename SizeOfElementFn>
    requires(detail::subitem_size_of<std::remove_cvref_t<SizeOfElementFn>, R>)
inline auto
item_size_of_map_indefinite(emit_context const &ctx,
                            R const &vs,
                            SizeOfElementFn &&sizeOfElement) noexcept
        -> std::uint64_t
{
    return detail::item_size_of_indefinite_array_like(
            ctx, vs, static_cast<SizeOfElementFn &&>(sizeOfElement));
}

} // namespace dplx::dp
