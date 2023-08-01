
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <concepts>
#include <ranges>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/emit_core.hpp>
#include <dplx/dp/items/type_code.hpp>
#include <dplx/dp/streams/output_buffer.hpp>

namespace dplx::dp
{

namespace detail
{

// clang-format off
template <typename Fn, typename R>
concept subitem_emitlet
        = requires(Fn &&encodeFn,
                   emit_context ctx,
                   std::ranges::range_reference_t<R const> v)
        {
            { static_cast<Fn &&>(encodeFn)(ctx, v) }
                -> cncr::tryable;
        };
// clang-format on

template <typename R, typename EncodeElementFn>
inline auto emit_array_like(emit_context &ctx,
                            R const &vs,
                            type_code type,
                            EncodeElementFn &&encodeElement) noexcept
        -> result<void>
{
    if constexpr (std::ranges::sized_range<R>)
    {
        detail::encodable_int auto const size = std::ranges::size(vs);
        assert(size >= 0);

        using code_type = detail::encoder_uint_t<std::ranges::range_size_t<R>>;
        DPLX_TRY(detail::store_var_uint<code_type>(
                ctx.out, static_cast<code_type>(size), type));

        for (auto &&v : vs)
        {
            DPLX_TRY(static_cast<EncodeElementFn &&>(encodeElement)(ctx, v));
        }
        return oc::success();
    }
    else
    {
        auto it = std::ranges::begin(vs);
        auto const end = std::ranges::end(vs);
        detail::encodable_int auto const size = std::ranges::distance(it, end);
        assert(size >= 0);

        using code_type = detail::encoder_uint_t<std::ranges::range_size_t<R>>;
        DPLX_TRY(detail::store_var_uint<code_type>(
                ctx.out, static_cast<code_type>(size), type));

        for (; it != end; ++it)
        {
            DPLX_TRY(static_cast<EncodeElementFn &&>(encodeElement)(ctx, *it));
        }
        return oc::success();
    }
}
template <typename R, typename EncodeElementFn>
inline auto emit_indefinite_array_like(emit_context &ctx,
                                       R const &vs,
                                       type_code type,
                                       EncodeElementFn &&encodeElement) noexcept
        -> result<void>
{
    DPLX_TRY(detail::store_inline_value(ctx.out, detail::indefinite_add_info,
                                        type));
    for (auto &&v : vs)
    {
        DPLX_TRY(static_cast<EncodeElementFn &&>(encodeElement)(ctx, v));
    }
    return dp::emit_break(ctx);
}

} // namespace detail

template <std::ranges::input_range R, typename EncodeElementFn>
    requires(std::ranges::forward_range<R> || std::ranges::sized_range<R>)
         && detail::subitem_emitlet<std::remove_cvref_t<EncodeElementFn>, R>
            inline auto emit_array(emit_context &ctx,
                                   R const &vs,
                                   EncodeElementFn &&encodeElement) noexcept
            -> result<void>
{
    return detail::emit_array_like(
            ctx, vs, type_code::array,
            static_cast<EncodeElementFn &&>(encodeElement));
}
template <std::ranges::input_range R, typename EncodeElementFn>
    requires detail::subitem_emitlet<std::remove_cvref_t<EncodeElementFn>, R>
inline auto emit_array_indefinite(emit_context &ctx,
                                  R const &vs,
                                  EncodeElementFn &&encodeElement) noexcept
        -> result<void>
{
    return detail::emit_indefinite_array_like(
            ctx, vs, type_code::array,
            static_cast<EncodeElementFn &&>(encodeElement));
}

template <std::ranges::input_range R, typename EncodeElementFn>
    requires(std::ranges::forward_range<R> || std::ranges::sized_range<R>)
         && detail::subitem_emitlet<std::remove_cvref_t<EncodeElementFn>, R>
            inline auto emit_map(emit_context &ctx,
                                 R const &vs,
                                 EncodeElementFn &&encodeElement) noexcept
            -> result<void>
{
    return detail::emit_array_like(
            ctx, vs, type_code::map,
            static_cast<EncodeElementFn &&>(encodeElement));
}
template <std::ranges::input_range R, typename EncodeElementFn>
    requires detail::subitem_emitlet<std::remove_cvref_t<EncodeElementFn>, R>
inline auto emit_map_indefinite(emit_context &ctx,
                                R const &vs,
                                EncodeElementFn &&encodeElement) noexcept
        -> result<void>
{
    return detail::emit_indefinite_array_like(
            ctx, vs, type_code::map,
            static_cast<EncodeElementFn &&>(encodeElement));
}

} // namespace dplx::dp
