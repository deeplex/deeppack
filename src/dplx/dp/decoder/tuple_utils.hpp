
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/dp/codecs/auto_tuple.hpp>
#include <dplx/dp/decoder/utils.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/item_parser.hpp>
#include <dplx/dp/layout_descriptor.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp
{

template <input_stream Stream, bool isVersioned = false>
inline auto parse_tuple_head(Stream &inStream,
                             std::bool_constant<isVersioned> = {})
        -> result<tuple_head_info>
{
    using parse = item_parser<Stream>;
    DPLX_TRY(auto &&arrayInfo, parse::generic(inStream));
    if (arrayInfo.type != type_code::array || arrayInfo.indefinite())
    {
        return errc::item_type_mismatch;
    }

    DPLX_TRY(auto &&remainingBytes, dp::available_input_size(inStream));
    if (arrayInfo.value > remainingBytes)
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

        // 0xffff'ffff => max() is reserved as null_def_version
        DPLX_TRY(auto version, parse::template integer<std::uint32_t>(
                                       inStream, 0xFFFF'FFFEU));

        return tuple_head_info{numProps - 1, version};
    }
}

template <auto const &descriptor, typename T, input_stream Stream>
inline auto decode_tuple_properties(Stream &stream,
                                    T &dest,
                                    std::int32_t numProperties) -> result<void>
{
    using decode_value_fn = detail::mp_decode_value_fn<T, Stream>;

    constexpr std::size_t expectedNumProps = descriptor.num_properties;
    if (numProperties != expectedNumProps)
    {
        return errc::tuple_size_mismatch;
    }

    DPLX_TRY(descriptor.mp_for_dots(decode_value_fn{stream, dest}));

    return oc::success();
}

template <packable_tuple T, input_stream Stream>
    requires(detail::versioned_decoder_enabled(layout_descriptor_for_v<T>))
class basic_decoder<T, Stream>
{
public:
    using value_type = T;

    inline auto operator()(Stream &inStream, value_type &dest) const
            -> result<void>
    {
        DPLX_TRY(auto &&headInfo,
                 dp::parse_tuple_head<Stream, layout_descriptor_for_v<T>.version
                                                      != null_def_version>(
                         inStream));

        if constexpr (layout_descriptor_for_v<T>.version != null_def_version)
        {
            if (layout_descriptor_for_v<T>.version != headInfo.version)
            {
                return errc::item_version_mismatch;
            }
        }

        return dp::decode_tuple_properties<layout_descriptor_for_v<T>, T,
                                           Stream>(inStream, dest,
                                                   headInfo.num_properties);
    }
};

} // namespace dplx::dp
