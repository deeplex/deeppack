
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <concepts>
#include <ranges>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/tag_invoke.hpp>

namespace dplx::dp
{

// output stream definitions & concepts

inline constexpr struct write_fn
{
    // direct encoding variant
    template <typename Stream>
    requires tag_invocable<write_fn, Stream &, std::size_t> auto
    operator()(Stream &stream, std::size_t size) const
        noexcept(nothrow_tag_invocable<write_fn, Stream &, std::size_t>)
            -> tag_invoke_result_t<write_fn, Stream &, std::size_t>
    {
        return ::dplx::dp::tag_invoke(*this, stream, size);
    }

    // bulk transfer variant (for payload data like strings)
    template <typename Stream>
    requires tag_invocable<write_fn,
                           Stream &,
                           std::byte const *,
                           std::size_t> auto
    operator()(Stream &stream,
               std::byte const *bytes,
               std::size_t numBytes) const
        noexcept(nothrow_tag_invocable<write_fn,
                                       Stream &,
                                       std::byte const *,
                                       std::size_t>)
            -> tag_invoke_result_t<write_fn,
                                   Stream &,
                                   std::byte const *,
                                   std::size_t>
    {
        return ::dplx::dp::tag_invoke(*this, stream, bytes, numBytes);
    }
} write{};

// clang-format off
template <typename T>
concept write_proxy
    = std::ranges::contiguous_range<T>
    && std::same_as<std::ranges::range_value_t<T>, std::byte>
    && requires(T &proxy, std::size_t const size)
    {
        { proxy.commit(size) } -> std::same_as<result<void>>;
    };
// clang-format on

// clang-format off
template <typename T>
concept write_result
    = oc::concepts::basic_result<T> && write_proxy<typename T::value_type>;
// clang-format on

// clang-format off
template <typename T>
concept output_stream
    = requires(T &stream, std::byte const *bytes, std::size_t const size)
    {
        {::dplx::dp::write(stream, size)} -> write_result;
        {::dplx::dp::write(stream, bytes, size)} -> oc::concepts::basic_result;
    };
// clang-format on

} // namespace dplx::dp
