
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
    //clang-format on
    inline auto operator()(OutStream &&outStream) const noexcept
            -> cncr::tag_invoke_result_t<get_output_buffer_fn, OutStream &&>
    {
        return cncr::tag_invoke(*this, static_cast<OutStream &&>(outStream));
    }

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
    //clang-format on
                inline auto
            operator()(InStream &&inStream) const noexcept
            -> cncr::tag_invoke_result_t<get_input_buffer_fn, InStream &&>
    {
        return cncr::tag_invoke(*this, static_cast<InStream &&>(inStream));
    }

} get_input_buffer{};

} // namespace dplx::dp
