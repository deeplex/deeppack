
// Copyright Henrik Steffen Gaßmann 2021.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <concepts>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/tag_invoke.hpp>

namespace dplx::dp
{

inline constexpr struct container_reserve_fn
{
    template <typename Container>
    requires nothrow_tag_invocable<container_reserve_fn,
                                   Container &,
                                   std::size_t const> auto
    operator()(Container &container,
               std::size_t const reservationSize) const noexcept
            -> tag_invoke_result_t<container_reserve_fn,
                                   Container &,
                                   std::size_t const>
    {
        return cpo::tag_invoke(*this, container, reservationSize);
    }

    template <typename StdContainer>
    friend inline auto tag_invoke(container_reserve_fn,
                                  StdContainer &container,
                                  std::size_t const reservationSize) noexcept
            -> result<void> requires requires
    {
        {
            container.reserve(reservationSize)
        }
        ->std::same_as<void>;
    }
    {
        try
        {
            container.reserve(reservationSize);
            return oc::success();
        }
        catch (std::bad_alloc const &)
        {
            return errc::not_enough_memory;
        }
    }
} container_reserve;

inline constexpr struct container_resize_fn
{
    template <typename Container>
    requires nothrow_tag_invocable<container_resize_fn,
                                   Container &,
                                   std::size_t const> auto
    operator()(Container &container, std::size_t const newSize) const noexcept
            -> tag_invoke_result_t<container_resize_fn,
                                   Container &,
                                   std::size_t const>
    {
        return cpo::tag_invoke(*this, container, newSize);
    }

    template <typename StdContainer>
    friend inline auto tag_invoke(container_resize_fn,
                                  StdContainer &container,
                                  std::size_t const newSize) noexcept
            -> result<void> requires requires
    {
        {
            container.resize(newSize)
        }
        ->std::same_as<void>;
    }
    {
        try
        {
            container.resize(newSize);
            return oc::success();
        }
        catch (std::bad_alloc const &)
        {
            return errc::not_enough_memory;
        }
    }

} container_resize;

inline constexpr struct container_resize_for_overwrite_fn
{
    template <typename Container>
    requires nothrow_tag_invocable<container_resize_for_overwrite_fn,
                                   Container &,
                                   std::size_t const> auto
    operator()(Container &container, std::size_t const newSize) const noexcept
            -> tag_invoke_result_t<container_resize_for_overwrite_fn,
                                   Container &,
                                   std::size_t const>
    {
        return cpo::tag_invoke(*this, container, newSize);
    }

    template <typename Container>
    requires(
            !nothrow_tag_invocable<
                    container_resize_for_overwrite_fn,
                    Container &,
                    std::size_t const> && tag_invocable<container_resize_fn, Container &, std::size_t const>) auto
    operator()(Container &container, std::size_t const newSize) const noexcept
            -> tag_invoke_result_t<container_resize_fn,
                                   Container &,
                                   std::size_t const>
    {
        return cpo::tag_invoke(container_resize, container, newSize);
    }

} container_resize_for_overwrite;

template <typename Container>
struct container_traits
{
    static constexpr bool reserve = nothrow_tag_invocable<container_reserve_fn,
                                                          Container &,
                                                          std::size_t const>;

    static constexpr bool resize = nothrow_tag_invocable<container_resize_fn,
                                                         Container &,
                                                         std::size_t const>;

    static constexpr bool resize_for_overwrite
            = resize
           || nothrow_tag_invocable<container_resize_for_overwrite_fn,
                                    Container &,
                                    std::size_t const>;
};

} // namespace dplx::dp
