
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/cncr/type_utils.hpp>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/emit_core.hpp>
#include <dplx/dp/layout_descriptor.hpp>
#include <dplx/dp/tuple_def.hpp>

namespace dplx::dp::detail
{

template <typename T>
struct mp_encode_tuple_member_fn
{
    emit_context const &ctx;
    T const &value;

    template <typename PropDefType>
    inline auto operator()(PropDefType const &propertyDef) -> result<void>
    {
        using element_type = typename PropDefType::value_type;

        return codec<element_type>::encode(
                ctx,
                static_cast<element_type const &>(propertyDef.access(value)));
    }
};

} // namespace dplx::dp::detail

namespace dplx::dp
{

template <auto const &descriptor>
inline auto
encode_tuple(emit_context const &ctx,
             detail::descriptor_class_type<descriptor> const &value) noexcept
        -> result<void>
{
    using encode_value_fn = detail::mp_encode_tuple_member_fn<
            detail::descriptor_class_type<descriptor>>;

    if constexpr (descriptor.version == null_def_version)
    {
        DPLX_TRY(dp::emit_array(ctx, descriptor.num_properties));
    }
    else
    {
        DPLX_TRY(dp::emit_array(ctx, descriptor.num_properties + 1));
        DPLX_TRY(dp::emit_integer(ctx, descriptor.version));
    }

    return descriptor.mp_for_dots(encode_value_fn{ctx, value});
}

template <packable_tuple T>
inline auto encode_tuple(emit_context const &ctx, T const &value) noexcept
        -> result<void>
{
    return dp::encode_tuple<layout_descriptor_for_v<T>>(ctx, value);
}

} // namespace dplx::dp
