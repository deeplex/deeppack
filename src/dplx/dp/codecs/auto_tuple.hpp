
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <type_traits>

#include <dplx/cncr/type_utils.hpp>

#include <dplx/dp/api.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/emit_core.hpp>
#include <dplx/dp/items/encoded_item_head_size.hpp>
#include <dplx/dp/items/item_size_of_core.hpp>
#include <dplx/dp/items/parse_context.hpp>
#include <dplx/dp/items/parse_core.hpp>
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
        return dp::encode(ctx,
                          static_cast<typename PropDefType::value_type const &>(
                                  propertyDef.access(value)));
    }
};

template <typename T>
struct mp_size_of_tuple_element_fn
{
    emit_context const &ctx;
    T const &value;

    template <typename PropDefType>
    constexpr auto operator()(PropDefType const &propertyDef) const noexcept
            -> std::uint64_t
    {
        return dp::encoded_size_of(
                ctx, static_cast<typename PropDefType::value_type const &>(
                             propertyDef.access(value)));
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

template <auto const &descriptor>
inline auto
size_of_tuple(emit_context const &ctx,
              detail::descriptor_class_type<descriptor> const &value) noexcept
        -> std::uint64_t
{
    using size_of_element_fn = detail::mp_size_of_tuple_element_fn<
            detail::descriptor_class_type<descriptor>>;

    std::uint64_t prefixSize = 0U;
    if constexpr (descriptor.version == null_def_version)
    {
        prefixSize += dp::encoded_item_head_size<type_code::array>(
                descriptor.num_properties);
    }
    else
    {
        prefixSize += dp::encoded_item_head_size<type_code::array>(
                descriptor.num_properties + 1U);
        prefixSize += dp::item_size_of_integer(ctx, descriptor.version);
    }

    return prefixSize
         + descriptor.mp_map_fold_left(size_of_element_fn{ctx, value});
}

template <packable_tuple T>
inline auto size_of_tuple(emit_context const &ctx, T const &value) noexcept
        -> std::uint64_t
{
    return dp::size_of_tuple<layout_descriptor_for_v<T>>(ctx, value);
}
} // namespace dplx::dp

namespace dplx::dp
{

namespace detail
{

template <typename T>
struct mp_decode_v_fn
{
    parse_context &ctx;
    T &dest;

    template <typename PropDefType>
    inline auto operator()(PropDefType const &propertyDef) const noexcept
            -> result<void>
    {
        return dp::decode(ctx, propertyDef.access(dest));
    }
};

} // namespace detail

struct tuple_head_info
{
    std::int32_t num_properties;
    std::uint32_t version;
};

template <bool isVersioned = false>
inline auto decode_tuple_head(parse_context &ctx,
                              std::bool_constant<isVersioned> = {}) noexcept
        -> result<tuple_head_info>
{
    auto &&parseHeadRx = dp::parse_item_head(ctx);
    if (parseHeadRx.has_error()) [[unlikely]]
    {
        return static_cast<decltype(parseHeadRx) &&>(parseHeadRx)
                .assume_error();
    }
    item_head const &arrayInfo = parseHeadRx.assume_value();
    if (arrayInfo.type != type_code::array || arrayInfo.indefinite())
    {
        return errc::item_type_mismatch;
    }

    if (ctx.in.input_size() < arrayInfo.value)
    {
        return errc::end_of_stream;
    }
    if (!std::in_range<std::int32_t>(arrayInfo.value))
    {
        return errc::too_many_properties;
    }
    auto numProps = static_cast<std::int32_t>(arrayInfo.value);

    if constexpr (!isVersioned)
    {
        return tuple_head_info{numProps, null_def_version};
    }
    else
    {
        if (arrayInfo.value < 1U)
        {
            return errc::item_version_property_missing;
        }

        std::uint32_t version; // NOLINT(cppcoreguidelines-init-variables)
        DPLX_TRY(dp::parse_integer(ctx, version, null_def_version - 1U));

        return tuple_head_info{numProps - 1, version};
    }
}

template <auto const &descriptor, typename T>
inline auto decode_tuple_properties(parse_context &ctx,
                                    T &dest,
                                    std::int32_t numProperties) noexcept
        -> result<void>
{
    constexpr std::size_t expectedNumProps = descriptor.num_properties;
    if (numProperties != expectedNumProps)
    {
        return errc::tuple_size_mismatch;
    }

    using decode_value_fn = detail::mp_decode_v_fn<T>;
    return descriptor.mp_for_dots(decode_value_fn{ctx, dest});
}

template <packable_tuple T>
inline auto decode_tuple(parse_context &ctx, T &value) noexcept -> result<void>
{
    DPLX_TRY(auto &&headInfo,
             dp::decode_tuple_head<layout_descriptor_for_v<T>.version
                                   != null_def_version>(ctx));

    if constexpr (layout_descriptor_for_v<T>.version != null_def_version)
    {
        if (layout_descriptor_for_v<T>.version != headInfo.version)
        {
            return errc::item_version_mismatch;
        }
    }

    return dp::decode_tuple_properties<layout_descriptor_for_v<T>, T>(
            ctx, value, headInfo.num_properties);
}

} // namespace dplx::dp
