
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <type_traits>

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/detail/mp_for_dots.hpp>
#include <dplx/dp/detail/mp_lite.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/encoder/api.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/item_emitter.hpp>
#include <dplx/dp/layout_descriptor.hpp>
#include <dplx/dp/stream.hpp>
#include <dplx/dp/tuple_def.hpp>

namespace dplx::dp::detail
{

template <auto const &descriptor, typename T, typename Stream>
struct mp_encode_value_fn
{
    Stream &stream;
    T const &value;

    template <std::size_t I>
    inline auto operator()(mp_size_t<I>) -> result<void>
    {
        constexpr auto &propertyDef = descriptor.template property<I>();

        using property_encoder = dp::basic_encoder<
                typename decltype(propertyDef.decl_value())::type, Stream>;

        DPLX_TRY(property_encoder()(stream, propertyDef.access(value)));
        return success();
    }
};

} // namespace dplx::dp::detail

namespace dplx::dp
{

template <auto const &descriptor, typename T, output_stream Stream>
inline auto encode_tuple(Stream &outStream, T const &value) -> result<void>
{
    using encode_value_fn = detail::mp_encode_value_fn<descriptor, T, Stream>;

    if constexpr (descriptor.version == null_def_version)
    {
        DPLX_TRY(item_emitter<Stream>::array(outStream,
                                             descriptor.num_properties));
    }
    else
    {
        DPLX_TRY(item_emitter<Stream>::array(outStream,
                                             descriptor.num_properties + 1));
        DPLX_TRY(item_emitter<Stream>::integer(outStream, descriptor.version));
    }

    DPLX_TRY(detail::mp_for_dots<descriptor.num_properties>(
            encode_value_fn{outStream, value}));

    return success();
}

template <auto const &descriptor, typename T, output_stream Stream>
inline auto encode_tuple(Stream &outStream,
                         T const &value,
                         std::uint32_t const version) -> result<void>
{
    using encode_value_fn = detail::mp_encode_value_fn<descriptor, T, Stream>;

    DPLX_TRY(item_emitter<Stream>::array(outStream,
                                         descriptor.num_properties + 1));
    DPLX_TRY(item_emitter<Stream>::integer(outStream, version));

    DPLX_TRY(detail::mp_for_dots<descriptor.num_properties>(
            encode_value_fn{outStream, value}));

    return success();
}

template <packable_tuple T, output_stream Stream>
class basic_encoder<T, Stream>
{
    static constexpr auto descriptor
            = layout_descriptor_for(std::type_identity<T>{});

public:
    using value_type = T;

    auto operator()(Stream &outStream, value_type const &value) const
            -> result<void>
    {
        return dp::encode_tuple<descriptor, T, Stream>(outStream, value);
    }
};

namespace detail
{

template <typename T>
struct encoded_size_of_tuple_member
{
    T const &value;

    constexpr auto operator()(auto const &propertyDef) const noexcept
    {
        return encoded_size_of(propertyDef.access(value));
    }
};

} // namespace detail

template <auto const &descriptor, typename T>
constexpr auto encoded_size_of_tuple(T const &value) noexcept -> std::size_t
{
    constexpr auto hasVersion = descriptor.version != null_def_version;

    auto const sizeOfProps = descriptor.mp_map_fold_left(
            detail::encoded_size_of_tuple_member<T>{value});

    auto const prefixSize = detail::var_uint_encoded_size(
            descriptor.num_properties + hasVersion);

    if constexpr (hasVersion)
    {
        return prefixSize + detail::var_uint_encoded_size(descriptor.version)
             + sizeOfProps;
    }
    else
    {
        return prefixSize + sizeOfProps;
    }
}

template <packable_tuple T>
constexpr auto tag_invoke(encoded_size_of_fn, T const &value) noexcept
        -> std::size_t 
{
    return dp::encoded_size_of_tuple<layout_descriptor_for_v<T>, T>(value);
}

} // namespace dplx::dp
