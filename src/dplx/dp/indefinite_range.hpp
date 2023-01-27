
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <ranges>
#include <type_traits>

namespace dplx::dp
{

template <std::input_iterator T, std::sentinel_for<T> S = T>
class indefinite_range : public std::ranges::view_base
{
    T mIt;
    S mEnd;

public:
    indefinite_range() noexcept = default;

    template <std::ranges::input_range R>
        requires std::assignable_from<T &,
                                      std::ranges::iterator_t<R const>> && std::
                assignable_from<S &, std::ranges::sentinel_t<R const>>
    constexpr explicit indefinite_range(R const &range)
        : mIt(std::ranges::begin(range))
        , mEnd(std::ranges::end(range))
    {
    }
    constexpr explicit indefinite_range(T it, S endIt)
        : mIt(static_cast<T &&>(it))
        , mEnd(static_cast<S &&>(endIt))
    {
    }

    [[nodiscard]] constexpr auto begin() const -> T
    {
        return mIt;
    }
    [[nodiscard]] constexpr auto end() const -> S
    {
        return mEnd;
    }
};

template <std::ranges::input_range R>
explicit indefinite_range(R const &)
        -> indefinite_range<std::ranges::iterator_t<R const>,
                            std::ranges::sentinel_t<R const>>;

} // namespace dplx::dp

template <typename T, typename S>
inline constexpr bool std::ranges::enable_borrowed_range<
        ::dplx::dp::indefinite_range<T, S>> = true;
