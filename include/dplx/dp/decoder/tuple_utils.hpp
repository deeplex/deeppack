
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/dp/decoder/utils.hpp>
#include <dplx/dp/detail/mp_for_dots.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/item_parser.hpp>
#include <dplx/dp/layout_descriptor.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp
{

struct tuple_head_info
{
    std::int32_t num_properties;
    std::uint32_t version;
};

template <input_stream Stream, bool isVersioned = false>
inline auto parse_tuple_head(Stream &inStream,
                             std::bool_constant<isVersioned> = {})
    -> result<tuple_head_info>
{
    DPLX_TRY(auto &&arrayInfo, detail::parse_item_info(inStream));
    if (std::byte{arrayInfo.type} != type_code::array)
    {
        return errc::item_type_mismatch;
    }

    DPLX_TRY(auto &&remainingBytes, dp::available_input_size(inStream));
    if (arrayInfo.value > remainingBytes)
    {
        return errc::end_of_stream;
    }
    if (arrayInfo.value >=
        static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
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
        if (arrayInfo.value < 1u)
        {
            return errc::item_version_property_missing;
        }

        DPLX_TRY(auto &&versionInfo, detail::parse_item_info(inStream));
        if (std::byte{versionInfo.type} != type_code::posint)
        {
            return errc::item_version_property_missing;
        }
        // 0xffff'ffff => max() is reserved as null_def_version
        if (versionInfo.value >= std::numeric_limits<std::uint32_t>::max())
        {
            return errc::item_value_out_of_range;
        }
        return tuple_head_info{numProps - 1,
                               static_cast<std::uint32_t>(versionInfo.value)};
    }
}

template <auto const &descriptor, typename T, input_stream Stream>
inline auto decode_tuple_properties(Stream &stream,
                                    T &dest,
                                    std::int32_t numProperties) -> result<void>
{
    using decode_value_fn = detail::mp_decode_value_fn<descriptor, T, Stream>;

    constexpr std::size_t expectedNumProps = descriptor.num_properties;
    if (numProperties != expectedNumProps)
    {
        return errc::tuple_size_mismatch;
    }

    DPLX_TRY(detail::mp_for_dots<descriptor.num_properties>(
        decode_value_fn{stream, dest}));

    return success();
}

template <packable_tuple T, input_stream Stream>
requires(detail::versioned_decoder_enabled(layout_descriptor_for(
    std::type_identity<T>{}))) class basic_decoder<T, Stream>
{
    static constexpr auto descriptor =
        layout_descriptor_for(std::type_identity<T>{});

public:
    using value_type = T;

    inline auto operator()(Stream &inStream, value_type &dest) const
        -> result<void>
    {
        DPLX_TRY(auto &&headInfo,
                 dp::parse_tuple_head<Stream,
                                      descriptor.version != null_def_version>(
                     inStream));

        if constexpr (descriptor.version != null_def_version)
        {
            if (descriptor.version != headInfo.version)
            {
                return errc::item_version_mismatch;
            }
        }

        return dp::decode_tuple_properties<descriptor, T, Stream>(
            inStream, dest, headInfo.num_properties);
    }
};

} // namespace dplx::dp
