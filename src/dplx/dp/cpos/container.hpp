
// Copyright Henrik Steffen Gaßmann 2021.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <new>

#include <dplx/cncr/tag_invoke.hpp>

#include <dplx/dp/disappointment.hpp>

namespace dplx::dp
{

inline constexpr struct container_reserve_fn
{
    template <typename Container>
        requires cncr::nothrow_tag_invocable<container_reserve_fn,
                                             Container &,
                                             std::size_t const>
    auto operator()(Container &container,
                    std::size_t const reservationSize) const noexcept
            -> cncr::tag_invoke_result_t<container_reserve_fn,
                                         Container &,
                                         std::size_t const>
    {
        return cncr::tag_invoke(*this, container, reservationSize);
    }

    template <typename StdContainer>
    friend inline auto tag_invoke(container_reserve_fn,
                                  StdContainer &container,
                                  std::size_t const reservationSize) noexcept
            -> result<void>
            // clang-format off
        requires requires
        {
            typename StdContainer;
            typename StdContainer::value_type;
            requires std::is_nothrow_move_constructible_v<
                typename StdContainer::value_type>;
            { container.reserve(reservationSize) }
                -> std::same_as<void>;
        }
    // clang-format on
    {
        try
        {
            container.reserve(reservationSize);
            return outcome::success();
        }
        catch (std::bad_alloc const &)
        {
            return errc::not_enough_memory;
        }
        catch (...)
        {
            std::terminate();
        }
    }
} container_reserve;

inline constexpr struct container_resize_fn
{
    template <typename Container>
        requires cncr::nothrow_tag_invocable<container_resize_fn,
                                             Container &,
                                             std::size_t const>
    auto operator()(Container &container,
                    std::size_t const newSize) const noexcept
            -> cncr::tag_invoke_result_t<container_resize_fn,
                                         Container &,
                                         std::size_t const>
    {
        return cncr::tag_invoke(*this, container, newSize);
    }

    template <typename StdContainer>
    friend inline auto tag_invoke(container_resize_fn,
                                  StdContainer &container,
                                  std::size_t const newSize) noexcept
            -> result<void>
            // clang-format off
        requires requires
        {
            typename StdContainer;
            typename StdContainer::value_type;
            requires std::is_nothrow_move_constructible_v<
                typename StdContainer::value_type>;
            { container.resize(newSize) }
                -> std::same_as<void>;
        }
    // clang-format on
    {
        try
        {
            container.resize(newSize);
            return outcome::success();
        }
        catch (std::bad_alloc const &)
        {
            return errc::not_enough_memory;
        }
        catch (...)
        {
            std::terminate();
        }
    }

} container_resize;

inline constexpr struct container_resize_for_overwrite_fn
{
    template <typename Container>
        requires cncr::nothrow_tag_invocable<container_resize_for_overwrite_fn,
                                             Container &,
                                             std::size_t const>
    auto operator()(Container &container,
                    std::size_t const newSize) const noexcept
            -> cncr::tag_invoke_result_t<container_resize_for_overwrite_fn,
                                         Container &,
                                         std::size_t const>
    {
        return cncr::tag_invoke(*this, container, newSize);
    }

    template <typename Container>
        requires(!cncr::nothrow_tag_invocable<container_resize_for_overwrite_fn,
                                              Container &,
                                              std::size_t const>
                 && cncr::tag_invocable<container_resize_fn,
                                        Container &,
                                        std::size_t const>)
    auto operator()(Container &container,
                    std::size_t const newSize) const noexcept
            -> cncr::tag_invoke_result_t<container_resize_fn,
                                         Container &,
                                         std::size_t const>
    {
        return cncr::tag_invoke(container_resize, container, newSize);
    }

} container_resize_for_overwrite;

template <typename Container>
struct container_traits
{
    static constexpr bool reserve
            = cncr::nothrow_tag_invocable<container_reserve_fn,
                                          Container &,
                                          std::size_t const>;

    static constexpr bool resize
            = cncr::nothrow_tag_invocable<container_resize_fn,
                                          Container &,
                                          std::size_t const>;

    static constexpr bool resize_for_overwrite
            = resize
           || cncr::nothrow_tag_invocable<container_resize_for_overwrite_fn,
                                          Container &,
                                          std::size_t const>;
};

} // namespace dplx::dp
