
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/dp/api.hpp>
#include <dplx/dp/concepts.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/emit_core.hpp>

namespace dplx::dp::ng
{

inline constexpr struct encode_varargs_fn final
{
    template <typename... Ts>
        requires(... &&encodable<cncr::remove_cref_t<Ts>>)
    inline auto operator()(emit_context const &ctx,
                           Ts &&...values) const noexcept -> result<void>
    {
        return bound_type(ctx)(static_cast<Ts &&>(values)...);
    }

    class bound_type final
    {
        emit_context const *mCtx;

    public:
        constexpr explicit bound_type(emit_context const &ctx)
            : mCtx(&ctx)
        {
        }

        template <typename... Ts>
            requires(... &&ng::encodable<cncr::remove_cref_t<Ts>>)
        inline auto operator()(Ts &&...vs) const noexcept -> result<void>
        {
            auto const &ctx = *mCtx;
            result<void> rx = dp::emit_array(ctx, sizeof...(Ts));

            if (!rx.has_failure()) [[likely]]
            {
                if constexpr (sizeof...(Ts) == 1U)
                {
                    rx = dp::encode(ctx, static_cast<Ts &&>(vs)...);
                }
                else if constexpr (sizeof...(Ts) > 1U)
                {
                    [[maybe_unused]] bool const failed
                            = (...
                               || detail::try_extract_failure(
                                       encode(ctx, static_cast<Ts &&>(vs)),
                                       rx));
                }
            }
            return rx;
        }
    };

    static constexpr auto bind(emit_context const &ctx) -> bound_type
    {
        return bound_type{ctx};
    }
} encode_varargs{};

constexpr struct encoded_size_of_varargs_fn
{
    template <typename... Ts>
        requires(... &&detail::has_codec_size_of<cncr::remove_cref_t<Ts>>)
    inline auto operator()(emit_context const &ctx,
                           Ts &&...values) const noexcept -> result<void>
    {
        return bound_type(ctx)(static_cast<Ts &&>(values)...);
    }

    class bound_type
    {
        emit_context const *mCtx;

    public:
        constexpr explicit bound_type(emit_context const &ctx)
            : mCtx(&ctx)
        {
        }

        template <typename... Ts>
            requires(... &&detail::has_codec_size_of<cncr::remove_cref_t<Ts>>)
        inline auto operator()(Ts &&...vs) const noexcept -> std::uint64_t
        {
            auto const &ctx = *mCtx;
            auto const headSize = dp::encoded_item_head_size(type_code::array,
                                                             sizeof...(Ts));

            return (headSize + ...
                    + dp::encoded_size_of(ctx, static_cast<Ts &&>(vs)));
        }
    };

    static constexpr auto bind(emit_context const &ctx) -> bound_type
    {
        return bound_type{ctx};
    }
} encoded_size_of_varargs;

} // namespace dplx::dp::ng

namespace dplx::dp::detail
{

template <typename T>
concept encodable_tuple_like2 = tuple_like<T> && requires(
        T const t, ng::encode_varargs_fn::bound_type const &enc)
{
    detail::apply_simply(enc, t);
};

} // namespace dplx::dp::detail

namespace dplx::dp
{

template <detail::encodable_tuple_like2 T>
class codec<T>
{
public:
    static auto size_of(emit_context const &ctx, T const &tuple) noexcept
            -> std::uint64_t
    {
        return detail::apply_simply(
                ng::encoded_size_of_varargs_fn::bound_type(ctx), tuple);
    }
    static auto encode(emit_context const &ctx, T const &tuple) noexcept
            -> result<void>
    {
        return detail::apply_simply(ng::encode_varargs_fn::bound_type(ctx),
                                    tuple);
    }
};

} // namespace dplx::dp
