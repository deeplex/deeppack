
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <concepts>

#include <dplx/cncr/tag_invoke.hpp>

#include <dplx/dp/fwd.hpp>

namespace dplx::dp
{

inline constexpr struct get_output_buffer_fn
{
    template <typename OutStream>
    // clang-format off
        requires cncr::nothrow_tag_invocable<get_output_buffer_fn, OutStream &&>
            && std::is_base_of_v<output_buffer,
                    std::remove_cvref_t<cncr::tag_invoke_result_t<get_output_buffer_fn, OutStream &&>>>
            && std::convertible_to<
                    std::add_lvalue_reference_t<cncr::tag_invoke_result_t<get_output_buffer_fn, OutStream &&>>,
                    output_buffer &>
    // clang-format on
    inline auto operator()(OutStream &&outStream) const noexcept
            -> cncr::tag_invoke_result_t<get_output_buffer_fn, OutStream &&>
    {
        return cncr::tag_invoke(*this, static_cast<OutStream &&>(outStream));
    }

    inline auto operator()(output_buffer &outStream) const noexcept
            -> output_buffer &
    {
        return outStream;
    }

    // poison pill to remove implicit conversions of "real" streams to some type
    // for which a get_output_buffer customization exists
    template <typename T>
        requires std::derived_from<std::remove_cvref_t<T>, output_buffer>
    friend inline void tag_invoke(get_output_buffer_fn, T &&) noexcept = delete;

} get_output_buffer{};

inline constexpr struct get_input_buffer_fn
{
    template <typename InStream>
    // clang-format off
        requires cncr::nothrow_tag_invocable<get_input_buffer_fn, InStream &&>
            && std::is_base_of_v<input_buffer,
                    std::remove_cvref_t<cncr::tag_invoke_result_t<get_input_buffer_fn, InStream &&>>>
            && std::convertible_to<
                    std::add_lvalue_reference_t<cncr::tag_invoke_result_t<get_input_buffer_fn, InStream &&>>,
                    input_buffer &>
    // clang-format on
    inline auto operator()(InStream &&inStream) const noexcept
            -> cncr::tag_invoke_result_t<get_input_buffer_fn, InStream &&>
    {
        return cncr::tag_invoke(*this, static_cast<InStream &&>(inStream));
    }

    inline auto operator()(input_buffer &inStream) const noexcept
            -> input_buffer &
    {
        return inStream;
    }
    template <typename T>
        requires(!cncr::tag_invocable<get_input_buffer_fn, T &&>)
                && std::is_rvalue_reference_v<T &&>
                && (!std::is_const_v<std::remove_reference_t<T>>)
                && std::movable<std::remove_reference_t<T>>
                && std::derived_from<std::remove_reference_t<T>, input_buffer>
    inline auto operator()(T &&inStream) const noexcept -> T
    {
        return static_cast<T &&>(inStream);
    }

    // poison pill to remove implicit conversions of "real" streams to some type
    // for which a get_input_buffer customization exists
    // e.g. memory_input_stream => std::span<std::byte const>
    template <typename T>
        requires std::derived_from<std::remove_cvref_t<T>, input_buffer>
    friend inline void tag_invoke(get_input_buffer_fn, T &&) noexcept = delete;

} get_input_buffer{};

} // namespace dplx::dp
