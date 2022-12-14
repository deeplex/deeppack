
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/emit_core.hpp>
#include <dplx/dp/layout_descriptor.hpp>
#include <dplx/dp/object_def.hpp>

namespace dplx::dp::detail
{

template <typename T>
struct mp_encode_object_property_fn2
{
    emit_context const &ctx;
    T const &value;

    template <typename PropDefType>
    inline auto operator()(PropDefType const &propertyDef) const noexcept
            -> result<void>
    {
        using key_type = typename PropDefType::id_type;
        using value_type = typename PropDefType::value_type;

        DPLX_TRY(codec<key_type>::encode(ctx, propertyDef.id));
        DPLX_TRY(codec<value_type>::encode(
                ctx,
                static_cast<value_type const &>(propertyDef.access(value))));
        return dp::success();
    }
};

} // namespace dplx::dp::detail

namespace dplx::dp
{

template <std::size_t N>
class codec<fixed_u8string<N>>
{
public:
    static auto encode(emit_context const &ctx,
                       fixed_u8string<N> const &value) noexcept -> result<void>
    {
        return dp::emit_u8string(ctx, value.data(), value.size());
    }
};

template <auto const &descriptor>
inline auto
encode_object(emit_context const &ctx,
              detail::descriptor_class_type<descriptor> const &value) noexcept
        -> result<void>
{
    using encode_property_fn = detail::mp_encode_object_property_fn2<
            detail::descriptor_class_type<descriptor>>;

    if constexpr (descriptor.version == null_def_version)
    {
        DPLX_TRY(dp::emit_map(ctx, descriptor.num_properties));
    }
    else
    {
        DPLX_TRY(dp::emit_map(ctx, descriptor.num_properties + 1));

        DPLX_TRY(
                dp::detail::store_inline_value(ctx.out, 0U, type_code::posint));
        DPLX_TRY(dp::emit_integer(ctx, descriptor.version));
    }

    return descriptor.mp_for_dots(encode_property_fn{ctx, value});
}

template <packable_object T>
inline auto encode_object(emit_context const &ctx, T const &value) noexcept
{
    return dp::encode_object<layout_descriptor_for_v<T>>(ctx, value);
}

} // namespace dplx::dp
