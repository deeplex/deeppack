
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>

#include <algorithm>
#include <array>
#include <compare>
#include <type_traits>
#include <utility>

#include <boost/mp11/algorithm.hpp>

#include <dplx/dp/decoder/std_string.hpp>
#include <dplx/dp/decoder/utils.hpp>
#include <dplx/dp/detail/hash.hpp>
#include <dplx/dp/detail/perfect_hash.hpp>
#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/layout_descriptor.hpp>
#include <dplx/dp/object_def.hpp>
#include <dplx/dp/tag_invoke.hpp>

namespace dplx::dp
{

inline constexpr struct property_id_hash_fn
{
    template <typename T>
    requires tag_invocable<property_id_hash_fn, T const &> constexpr auto
    operator()(T const &value) const
        noexcept(nothrow_tag_invocable<property_id_hash_fn, T const &>)
            -> std::uint64_t
    {
        return ::dplx::dp::cpo::tag_invoke(*this, value);
    }
    template <typename T>
    requires tag_invocable<property_id_hash_fn,
                           T const &,
                           std::uint64_t> constexpr auto
    operator()(T const &value, std::uint64_t seed) const noexcept(
        nothrow_tag_invocable<property_id_hash_fn, T const &, std::uint64_t>)
        -> std::uint64_t
    {
        return ::dplx::dp::cpo::tag_invoke(*this, value, seed);
    }

    template <integer T>
    friend constexpr auto tag_invoke(property_id_hash_fn, T value) noexcept
        -> std::uint64_t
    {
        return static_cast<std::uint64_t>(value);
    }
    template <integer T>
    friend constexpr auto
    tag_invoke(property_id_hash_fn, T value, std::uint64_t seed) noexcept
        -> std::uint64_t
    {
        return detail::xxhash3(value, seed);
    }

    friend constexpr auto tag_invoke(property_id_hash_fn,
                                     std::u8string_view str,
                                     std::uint64_t const seed = 0) noexcept
        -> std::uint64_t
    {
        return detail::fnvx_hash(str.data(), str.size(), seed);
    }

} property_id_hash;

template <std::size_t N, input_stream Stream>
class basic_decoder<fixed_u8string<N>, Stream>
{
public:
    auto operator()(Stream &inStream, fixed_u8string<N> &out) const
        -> result<void>
    {
        DPLX_TRY(strInfo, detail::parse_item_info(inStream));

        if (std::byte{strInfo.type} != type_code::text)
        {
            return errc::item_type_mismatch;
        }
        if (strInfo.value > out.max_size())
        {
            return errc::item_value_out_of_range;
        }
        out.mNumCodeUnits = static_cast<unsigned int>(strInfo.value);

        DPLX_TRY(availableBytes, dp::available_input_size(inStream));
        if (availableBytes < out.size())
        {
            return errc::missing_data;
        }

        DPLX_TRY(dp::read(
            inStream, reinterpret_cast<std::byte *>(out.data()), out.size()));
        return success();
    }
};

} // namespace dplx::dp

namespace dplx::dp::detail
{

template <std::size_t NumBits>
constexpr auto compress_bitset(std::initializer_list<bool> vs) noexcept
{
    constexpr auto digits = static_cast<std::size_t>(digits_v<std::size_t>);

    constexpr auto numBuckets = detail::div_ceil(NumBits, digits);
    std::array<std::size_t, numBuckets> buckets{};

    auto it = vs.begin();
    auto const end = vs.end();
    for (std::size_t shift = 0, offset = 0; it != end; ++it, ++shift)
    {
        if (shift == digits)
        {
            shift = 0;
            offset += 1;
        }

        buckets[offset] |= static_cast<std::size_t>(*it) << shift;
    }
    return buckets;
}

template <template <auto...> typename ObjectDefLike, auto... Properties>
constexpr auto
compress_optional_props(ObjectDefLike<Properties...> const &) noexcept
{
    return detail::compress_bitset<sizeof...(Properties)>(
        {Properties.required...});
}

template <auto const &descriptor>
inline constexpr auto
    required_prop_mask_for = detail::compress_optional_props(descriptor);

inline constexpr std::size_t unknown_property_id = ~static_cast<std::size_t>(0);

template <typename IdType, std::size_t NumIds, bool use_perfect_hash>
struct property_id_lookup_fn;

template <typename IdType, std::size_t NumIds>
struct property_id_lookup_fn<IdType, NumIds, true>
{
private:
    using id_type = IdType;
    using array_type = std::array<id_type, NumIds>;

    perfect_hasher<id_type, NumIds, property_id_hash_fn> hash;
    array_type const &ids;

public:
    constexpr property_id_lookup_fn(array_type const &ids)
        : hash(ids)
        , ids(ids)
    {
    }

    template <typename TLike>
    constexpr auto operator()(TLike &&id) const noexcept -> std::size_t
    {
        std::size_t const idx = hash(id);
        if (ids[idx] != id)
        {
            return unknown_property_id;
        }
        return idx;
    }
};

template <typename IdType, std::size_t NumIds>
struct property_id_lookup_fn<IdType, NumIds, false>
{
private:
    using id_type = IdType;
    using array_type = std::array<id_type, NumIds>;

    array_type const &ids;

public:
    constexpr property_id_lookup_fn(array_type const &ids)
        : ids(ids)
    {
    }

    template <typename TLike>
    constexpr auto operator()(TLike &&id) const noexcept -> std::size_t
    {
        auto const begin = ids.data();
        auto const end = ids.data() + ids.size();
        id_type const *it;
        if constexpr (NumIds <= 64)
        {
            if (it = std::find(begin, end, id); it == end)
            {
                return unknown_property_id;
            }
        }
        else
        {
            if (it = std::lower_bound(begin, end, id); it == end || *it != id)
            {
                return unknown_property_id;
            }
        }
        return static_cast<std::size_t>(it - begin);
    }
};

template <auto const &Descriptor, typename T, input_stream Stream>
class decode_object_property_fn
{
    static constexpr auto const &descriptor = Descriptor;
    static constexpr auto ids = descriptor.ids;
    static constexpr std::size_t const num_prop_ids = descriptor.ids.size();

#if !BOOST_PREDEF_WORKAROUND(BOOST_COMP_GNUC, <=, 10, 1, 0)
    static_assert(std::is_sorted(descriptor.ids.begin(), descriptor.ids.end()));
#endif

    using odef_type = std::remove_cvref_t<decltype(descriptor)>;
    using id_type = typename odef_type::id_type;
    using id_runtime_type = typename odef_type::id_runtime_type;
    using id_decoder = basic_decoder<id_runtime_type, Stream>;

    static constexpr property_id_lookup_fn<id_type,
                                           descriptor.ids.size(),
                                           false>
        lookup{descriptor.ids};

    using decode_value_fn = mp_decode_value_fn<descriptor, T, Stream>;

public:
    auto operator()(Stream &inStream, T &dest) const -> result<std::size_t>
    {
        id_runtime_type id{};
        DPLX_TRY(id_decoder()(inStream, id));

        auto const idx = lookup(id);
        if (idx == unknown_property_id)
        {
            return errc::unknown_property;
        }

        return boost::mp11::mp_with_index<num_prop_ids>(
            idx, decode_value_fn{inStream, dest});
    }
};

template <auto const &Descriptor, typename T, input_stream Stream>
inline constexpr decode_object_property_fn<Descriptor, T, Stream>
    decode_object_property{};

template <typename T>
constexpr auto index_of_limit(T const *elems,
                              std::size_t const num,
                              T const limit) noexcept -> std::size_t
{
    for (std::size_t i = 0; i < num; ++i)
    {
        if (elems[i] >= limit)
        {
            return i;
        }
    }
    return num;
}

template <typename T, decltype(auto) Descriptor, typename Stream>
requires dp::unsigned_integer<typename std::remove_cvref_t<decltype(
    Descriptor)>::id_type> class decode_object_property_fn<Descriptor,
                                                           T,
                                                           Stream>
{
    using descriptor_type = std::remove_cvref_t<decltype(Descriptor)>;
    using id_type = typename descriptor_type::id_type;
    static constexpr descriptor_type const &descriptor = Descriptor;

#if !BOOST_PREDEF_WORKAROUND(BOOST_COMP_GNUC, <=, 10, 1, 0)
    static_assert(std::is_sorted(descriptor.ids.begin(), descriptor.ids.end()));
#endif
    static_assert(detail::digits_v<id_type> <= 64);

    static constexpr id_type small_id_limit = detail::inline_value_max + 1;
    static constexpr auto small_ids_end = detail::index_of_limit(
        descriptor.ids.data(), descriptor.ids.size(), small_id_limit);

    static constexpr std::size_t id_map_size =
        descriptor.ids.size() - small_ids_end;

    static constexpr auto copy_large_ids() noexcept
        -> std::array<id_type, id_map_size>
    {
        std::array<id_type, id_map_size> ids{};
        for (std::size_t i = 0; i < id_map_size; ++i)
        {
            ids[i] = descriptor.ids[i + small_ids_end];
        }
        return ids;
    }
    static constexpr auto large_ids = copy_large_ids();
    static constexpr property_id_lookup_fn<id_type, id_map_size, false> lookup{
        large_ids};

    using decode_value_fn = mp_decode_value_fn<descriptor, T, Stream>;

    struct decode_prop_small_id_fn : decode_value_fn
    {
        template <std::size_t I>
        auto operator()(boost::mp11::mp_size_t<I>) -> result<std::size_t>
        {
            constexpr std::size_t propPos = static_cast<std::size_t>(
                std::find(descriptor.ids.data(),
                          descriptor.ids.data() + small_ids_end,
                          static_cast<id_type>(I)) -
                descriptor.ids.data());

            if constexpr (propPos == small_ids_end)
            {
                return errc::unknown_property;
            }
            else
            {
                return decode_value_fn::operator()(
                    boost::mp11::mp_size_t<propPos>{});
            }
        }
    };

    struct decode_prop_large_id_fn : public decode_value_fn
    {
        template <std::size_t I>
        auto operator()(boost::mp11::mp_size_t<I>) -> result<std::size_t>
        {
            return decode_value_fn::operator()(
                boost::mp11::mp_size_t<I + small_ids_end>{});
        }
    };

public:
    auto operator()(Stream &inStream, T &dest) const -> result<std::size_t>
    {
        DPLX_TRY(idInfo, detail::parse_item_info(inStream));
        if (std::byte{idInfo.type} != type_code::posint)
        {
            return errc::item_type_mismatch;
        }
        if (idInfo.value > std::numeric_limits<id_type>::max())
        {
            return errc::unknown_property;
        }

        if (idInfo.value < small_id_limit)
        {
            if constexpr (small_ids_end == 0)
            {
                return errc::unknown_property;
            }
            else
            {
                return boost::mp11::mp_with_index<small_id_limit>(
                    static_cast<std::size_t>(idInfo.value),
                    decode_prop_small_id_fn{inStream, dest});
            }
        }
        else
        {
            if constexpr (small_ids_end == descriptor.ids.size())
            {
                return errc::unknown_property;
            }
            else
            {
                auto const idx = lookup(static_cast<id_type>(idInfo.value));
                if (idx == unknown_property_id)
                {
                    return errc::unknown_property;
                }

                return boost::mp11::mp_with_index<id_map_size>(
                    idx, decode_prop_large_id_fn{inStream, dest});
            }
        }
    }
};

} // namespace dplx::dp::detail

namespace dplx::dp
{

struct object_head_info
{
    std::int32_t num_properties;
    std::uint32_t version;
};

template <input_stream Stream, bool isVersioned = true>
inline auto parse_object_head(Stream &inStream,
                              std::bool_constant<isVersioned> = {})
    -> result<object_head_info>
{
    DPLX_TRY(mapInfo, detail::parse_item_info(inStream));
    if (static_cast<std::byte>(mapInfo.type & 0b111'00000) != type_code::map)
    {
        return errc::item_type_mismatch;
    }
    auto const indefinite = mapInfo.indefinite();
    if (!indefinite && mapInfo.value == 0)
    {
        return object_head_info{0, null_def_version};
    }

    DPLX_TRY(remainingBytes, dp::available_input_size(inStream));
    // every prop consists of two items each being at least 1B big
    if (mapInfo.value > (remainingBytes / 2))
    {
        return errc::end_of_stream;
    }
    if (mapInfo.value >= static_cast<std::uint64_t>(
                             std::numeric_limits<std::int32_t>::max() / 2))
    {
        return errc::too_many_properties;
    }
    auto numProps = static_cast<std::int32_t>(mapInfo.value);

    if constexpr (!isVersioned)
    {
        return object_head_info{numProps, null_def_version};
    }
    else
    {
        // the version property id is posint 0
        // and always encoded as a single byte
        DPLX_TRY(maybeVersionReadProxy, dp::read(inStream, 1));
        if (std::ranges::data(maybeVersionReadProxy)[0] != std::byte{})
        {
            DPLX_TRY(dp::consume(inStream, maybeVersionReadProxy, 0));
            return object_head_info{numProps, null_def_version};
        }

        if constexpr (dp::lazy_input_stream<Stream>)
        {
            DPLX_TRY(dp::consume(inStream, maybeVersionReadProxy));
        }

        DPLX_TRY(versionInfo, detail::parse_item_info(inStream));
        if (std::byte{versionInfo.type} != type_code::posint)
        {
            return errc::item_type_mismatch;
        }
        // 0xffff'ffff => max() is reserved as null_def_version
        if (versionInfo.value >= std::numeric_limits<std::uint32_t>::max())
        {
            return errc::item_value_out_of_range;
        }
        return object_head_info{numProps - 1,
                                static_cast<std::uint32_t>(versionInfo.value)};
    }
}

template <auto const &descriptor, typename T, input_stream Stream>
inline auto decode_object_property(Stream &stream, T &dest)
    -> result<std::size_t>
{
    return detail::decode_object_property<descriptor, T, Stream>(stream, dest);
}

template <auto const &descriptor, typename T, input_stream Stream>
inline auto decode_object_properties(Stream &stream,
                                     T &dest,
                                     std::int32_t numProperties) -> result<void>
{
    constexpr decltype(auto) decode_object_property =
        detail::decode_object_property<descriptor, T, Stream>;

    if constexpr (descriptor.has_optional_properties)
    {
        std::array<std::size_t,
                   detail::required_prop_mask_for<descriptor>.size()>
            foundProps{};

        for (std::int32_t i = 0; i < numProperties; ++i)
        {
            DPLX_TRY(which, decode_object_property(stream, dest));

            auto const offset = which / detail::digits_v<std::size_t>;
            auto const shift = which % detail::digits_v<std::size_t>;

            foundProps[offset] |= static_cast<std::size_t>(1) << shift;
        }

        std::size_t acc = 0;
        for (std::size_t i = 0; i < foundProps.size(); ++i)
        {
            auto const requiredProps =
                detail::required_prop_mask_for<descriptor>[i];

            acc += (foundProps[i] & requiredProps) == requiredProps;
        }
        if (acc != foundProps.size())
        {
            return errc::required_object_property_missing;
        }
    }
    else
    {
        if (descriptor.num_properties != numProperties)
        {
            return errc::required_object_property_missing;
        }

        for (std::int32_t i = 0; i < numProperties; ++i)
        {
            DPLX_TRY(decode_object_property(stream, dest));
        }
    }
    return success();
}

template <packable_object T, input_stream Stream>
requires(detail::versioned_decoder_enabled(layout_descriptor_for(
    std::type_identity<T>{}))) class basic_decoder<T, Stream>
{
    static constexpr auto descriptor =
        layout_descriptor_for(std::type_identity<T>{});

public:
    auto operator()(Stream &inStream, T &dest) const -> result<void>
    {
        DPLX_TRY(headInfo,
                 dp::parse_object_head<Stream,
                                       descriptor.version != null_def_version>(
                     inStream));

        if constexpr (descriptor.version != null_def_version)
        {
            if (descriptor.version != headInfo.version)
            {
                return errc::item_version_mismatch;
            }
        }

        return dp::decode_object_properties<descriptor, T, Stream>(
            inStream, dest, headInfo.num_properties);
    }
};

} // namespace dplx::dp
