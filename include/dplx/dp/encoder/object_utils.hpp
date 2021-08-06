
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/detail/mp_lite.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/encoder/api.hpp>
#include <dplx/dp/item_emitter.hpp>
#include <dplx/dp/layout_descriptor.hpp>
#include <dplx/dp/object_def.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp::detail
{

template <typename T, typename Stream>
struct mp_encode_object_property_fn
{
    Stream &outStream;
    T const &value;

    template <typename PropDefType>
    inline auto operator()(PropDefType const &propertyDef) -> result<void>
    {
        using key_encoder
                = basic_encoder<typename PropDefType::id_type, Stream>;
        using value_encoder
                = basic_encoder<typename PropDefType::value_type, Stream>;

        DPLX_TRY(key_encoder()(outStream, propertyDef.id));
        DPLX_TRY(value_encoder()(outStream, propertyDef.access(value)));
        return oc::success();
    }
};

} // namespace dplx::dp::detail

namespace dplx::dp
{

template <std::size_t N, output_stream Stream>
class basic_encoder<fixed_u8string<N>, Stream>
{
public:
    using value_type = fixed_u8string<N>;

    auto operator()(Stream &outStream, value_type const &value) const
            -> result<void>
    {
        DPLX_TRY(item_emitter<Stream>::u8string(outStream, value.size()));

        DPLX_TRY(write(outStream,
                       reinterpret_cast<std::byte const *>(value.data()),
                       value.size()));

        return success();
    }
};

template <auto const &descriptor, typename T, output_stream Stream>
inline auto encode_object(Stream &outStream, T const &value) -> result<void>
{
    using emit = item_emitter<Stream>;
    using encode_property_fn = detail::mp_encode_object_property_fn<T, Stream>;

    if constexpr (descriptor.version == null_def_version)
    {
        DPLX_TRY(emit::map(outStream, descriptor.num_properties));
    }
    else
    {
        DPLX_TRY(emit::map(outStream, descriptor.num_properties + 1));

        DPLX_TRY(auto &&versionIdWriteProxy, write(outStream, 1));
        std::ranges::data(versionIdWriteProxy)[0] = std::byte{};
        if constexpr (lazy_output_stream<Stream>)
        {
            DPLX_TRY(commit(outStream, versionIdWriteProxy));
        }

        DPLX_TRY(emit::integer(outStream, descriptor.version));
    }

    DPLX_TRY(descriptor.mp_for_dots(encode_property_fn{outStream, value}));

    return success();
}

template <auto const &descriptor, typename T, output_stream Stream>
inline auto encode_object(Stream &outStream,
                          T const &value,
                          std::uint32_t version) -> result<void>
{
    using encode_property_fn = detail::mp_encode_object_property_fn<T, Stream>;

    DPLX_TRY(item_emitter<Stream>::map(outStream,
                                       descriptor.num_properties + 1));

    {
        DPLX_TRY(auto &&versionIdWriteProxy, write(outStream, 1));
        std::ranges::data(versionIdWriteProxy)[0] = std::byte{};

        if constexpr (lazy_output_stream<Stream>)
        {
            DPLX_TRY(commit(outStream, versionIdWriteProxy));
        }
    }
    DPLX_TRY(item_emitter<Stream>::integer(outStream, version));

    DPLX_TRY(descriptor.mp_for_dots(encode_property_fn{outStream, value}));

    return success();
}

template <packable_object T, output_stream Stream>
class basic_encoder<T, Stream>
{
public:
    using value_type = T;

    auto operator()(Stream &outStream, value_type const &value) const
            -> result<void>
    {
        return dp::encode_object<layout_descriptor_for_v<T>, T, Stream>(
                outStream, value);
    }
};

namespace detail
{

template <typename T>
struct encoded_size_of_property
{
    T const &value;

    constexpr auto operator()(auto const &propertyDef) const noexcept
    {
        auto const idSize = encoded_size_of(propertyDef.id);
        auto const valueSize = encoded_size_of(propertyDef.access(value));
        return idSize + valueSize;
    }
};

} // namespace detail

template <auto const &descriptor, typename T>
inline constexpr auto encoded_size_of_object(T const &value) noexcept
        -> std::size_t
{
    constexpr auto hasVersion = descriptor.version != null_def_version;

    auto const sizeOfProps = descriptor.mp_map_fold_left(
            detail::encoded_size_of_property<T>{value});

    auto const prefixSize = detail::var_uint_encoded_size(
            descriptor.num_properties + hasVersion);

    if constexpr (hasVersion)
    {
        return prefixSize + 1u + encoded_size_of(descriptor.version)
             + sizeOfProps;
    }
    else
    {
        return prefixSize + sizeOfProps;
    }
}

template <packable_object T>
inline constexpr auto tag_invoke(encoded_size_of_fn, T const &value) noexcept
        -> std::size_t
{
    return dp::encoded_size_of_object<layout_descriptor_for_v<T>, T>(value);
}

} // namespace dplx::dp
