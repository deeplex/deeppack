
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <algorithm>
#include <array>
#include <compare>
#include <cstdint>
#include <type_traits>
#include <utility>

#include <boost/mp11/algorithm.hpp>

#include <dplx/cncr/math_supplement.hpp>
#include <dplx/cncr/tag_invoke.hpp>
#include <dplx/cncr/type_utils.hpp>

#include <dplx/dp/decoder/api.hpp>
#include <dplx/dp/decoder/std_string.hpp>
#include <dplx/dp/decoder/utils.hpp>
#include <dplx/dp/detail/hash.hpp>
#include <dplx/dp/detail/perfect_hash.hpp>
#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/layout_descriptor.hpp>
#include <dplx/dp/object_def.hpp>

namespace dplx::dp
{

inline constexpr struct property_id_hash_fn
{
    template <typename T>
        requires cncr::tag_invocable<property_id_hash_fn, T const &>
    constexpr auto operator()(T const &value) const noexcept(
            cncr::nothrow_tag_invocable<property_id_hash_fn, T const &>)
            -> std::uint64_t
    {
        return cncr::tag_invoke(*this, value);
    }
    template <typename T>
        requires cncr::
                tag_invocable<property_id_hash_fn, T const &, std::uint64_t>
    constexpr auto operator()(T const &value, std::uint64_t seed) const
            noexcept(cncr::nothrow_tag_invocable<property_id_hash_fn,
                                                  T const &,
                                                  std::uint64_t>)
                    -> std::uint64_t
    {
        return cncr::tag_invoke(*this, value, seed);
    }

    template <cncr::integer T>
    friend constexpr auto tag_invoke(property_id_hash_fn, T value) noexcept
            -> std::uint64_t
    {
        return static_cast<std::uint64_t>(value);
    }
    template <cncr::integer T>
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
    using parse = item_parser<Stream>;

public:
    auto operator()(Stream &inStream, fixed_u8string<N> &out) const
            -> result<void>
    {
        DPLX_TRY(parse::u8string_finite(inStream, out, N));
        return oc::success();
    }
};

} // namespace dplx::dp

namespace dplx::dp::detail
{

template <std::size_t NumBits>
constexpr auto compress_bitset(std::initializer_list<bool> vs) noexcept
{
    constexpr auto digits = static_cast<std::size_t>(digits_v<std::size_t>);

    constexpr auto numBuckets = cncr::div_ceil(NumBits, digits);
    std::array<std::size_t, numBuckets> buckets{};

    auto const *it = vs.begin();
    auto const *const end = vs.end();
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
inline constexpr auto required_prop_mask_for
        = detail::compress_optional_props(descriptor);

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
    constexpr property_id_lookup_fn(array_type const &idsInit)
        : hash(ids)
        , ids(idsInit)
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
    constexpr property_id_lookup_fn(array_type const &idsInit)
        : ids(idsInit)
    {
    }

    template <typename TLike>
    constexpr auto operator()(TLike &&id) const noexcept -> std::size_t
    {
        auto const *const begin = ids.data();
        auto const *const end = ids.data() + ids.size();
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

#if DPLX_DP_WORKAROUND(DPLX_COMP_GNUC, <=, 10, 1, 0)
    // fixed_u8string id comparison operator is borked
#else
    static_assert(std::is_sorted(descriptor.ids.begin(), descriptor.ids.end()));
#endif

    using odef_type = cncr::remove_cref_t<decltype(descriptor)>;
    using id_type = typename odef_type::id_type;
    using id_runtime_type = typename odef_type::id_runtime_type;

    static constexpr property_id_lookup_fn<id_type,
                                           descriptor.ids.size(),
                                           false>
            lookup{descriptor.ids};

    using decode_value_fn = mp_decode_value_fn<T, Stream>;

    struct decode_prop_fn : public decode_value_fn
    {
        template <std::size_t I>
        auto operator()(boost::mp11::mp_size_t<I>) -> result<std::size_t>
        {
            constexpr auto &propertyDef = descriptor.template property<I>();
            DPLX_TRY(decode_value_fn::operator()(propertyDef));
            return I;
        }
    };

public:
    auto operator()(Stream &inStream, T &dest) const -> result<std::size_t>
    {
        DPLX_TRY(auto &&id, decode(as_value<id_runtime_type>, inStream));

        auto const idx = lookup(id);
        if (idx == unknown_property_id)
        {
            return errc::unknown_property;
        }

        return boost::mp11::mp_with_index<num_prop_ids>(
                idx, decode_prop_fn{
                             {inStream, dest}
        });
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

template <auto const &descriptor, typename T, input_stream Stream>
    requires cncr::unsigned_integer<
            typename cncr::remove_cref_t<decltype(descriptor)>::id_type>
class decode_object_property_fn<descriptor, T, Stream>
{
    using parse = item_parser<Stream>;
    using descriptor_type = cncr::remove_cref_t<decltype(descriptor)>;
    using id_type = typename descriptor_type::id_type;

#if DPLX_DP_WORKAROUND(DPLX_COMP_GNUC, <=, 10, 1, 0)
    // fixed_u8string id comparison operator is borked
#else
    static_assert(std::is_sorted(descriptor.ids.begin(), descriptor.ids.end()));
#endif
    static_assert(detail::digits_v<id_type> <= detail::digits_v<std::uint64_t>);

    static constexpr id_type small_id_limit = detail::inline_value_max + 1;
    static constexpr auto small_ids_end = detail::index_of_limit(
            descriptor.ids.data(), descriptor.ids.size(), small_id_limit);

    static constexpr std::size_t id_map_size
            = descriptor.ids.size() - small_ids_end;

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

    using decode_value_fn = mp_decode_value_fn<T, Stream>;

    struct decode_prop_small_id_fn : decode_value_fn
    {
        template <std::size_t I>
        auto operator()(boost::mp11::mp_size_t<I>) -> result<std::size_t>
        {
            constexpr auto propPos = static_cast<std::size_t>(
                    std::find(descriptor.ids.data(),
                              descriptor.ids.data() + small_ids_end,
                              static_cast<id_type>(I))
                    - descriptor.ids.data());

            if constexpr (propPos == small_ids_end)
            {
                return errc::unknown_property;
            }
            else
            {
                constexpr auto &propertyDef
                        = descriptor.template property<propPos>();
                DPLX_TRY(decode_value_fn::operator()(propertyDef));
                return propPos;
            }
        }
    };

    struct decode_prop_large_id_fn : public decode_value_fn
    {
        template <std::size_t I>
        auto operator()(boost::mp11::mp_size_t<I>) -> result<std::size_t>
        {
            constexpr auto &propertyDef
                    = descriptor.template property<I + small_ids_end>();
            DPLX_TRY(decode_value_fn::operator()(propertyDef));
            return I + small_ids_end;
        }
    };

public:
    auto operator()(Stream &inStream, T &dest) const -> result<std::size_t>
    {
        DPLX_TRY(auto id, parse::template integer<id_type>(inStream));

        if (id < small_id_limit)
        {
            if constexpr (small_ids_end == 0)
            {
                return errc::unknown_property;
            }
            else
            {
                return boost::mp11::mp_with_index<small_id_limit>(
                        static_cast<std::size_t>(id),
                        decode_prop_small_id_fn{
                                {inStream, dest}
                });
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
                auto const idx = lookup(static_cast<id_type>(id));
                if (idx == unknown_property_id)
                {
                    return errc::unknown_property;
                }

                return boost::mp11::mp_with_index<id_map_size>(
                        idx, decode_prop_large_id_fn{
                                     {inStream, dest}
                });
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
    using parse = item_parser<Stream>;
    DPLX_TRY(auto &&mapInfo, parse::generic(inStream));
    if (mapInfo.type != type_code::map || mapInfo.indefinite())
    {
        return errc::item_type_mismatch;
    }
    if (mapInfo.value == 0)
    {
        return object_head_info{0, null_def_version};
    }

    DPLX_TRY(auto &&remainingBytes, dp::available_input_size(inStream));
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
        DPLX_TRY(auto &&maybeVersionReadProxy, dp::read(inStream, 1));
        if (std::ranges::data(maybeVersionReadProxy)[0] != std::byte{})
        {
            DPLX_TRY(dp::consume(inStream, maybeVersionReadProxy, 0));
            return object_head_info{numProps, null_def_version};
        }

        if constexpr (dp::lazy_input_stream<Stream>)
        {
            DPLX_TRY(dp::consume(inStream, maybeVersionReadProxy));
        }

        // 0xffff'ffff => max() is reserved as null_def_version
        DPLX_TRY(auto version, parse::template integer<std::uint32_t>(
                                       inStream, 0xffff'fffeU));

        return object_head_info{numProps - 1, version};
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
    constexpr auto &decode_object_property
            = detail::decode_object_property<descriptor, T, Stream>;

    if constexpr (descriptor.has_optional_properties)
    {
        std::array<std::size_t,
                   detail::required_prop_mask_for<descriptor>.size()>
                foundProps{};

        for (std::int32_t i = 0; i < numProperties; ++i)
        {
            DPLX_TRY(auto &&which, decode_object_property(stream, dest));

            auto const offset = which / detail::digits_v<std::size_t>;
            auto const shift = which % detail::digits_v<std::size_t>;

            foundProps[offset] |= static_cast<std::size_t>(1) << shift;
        }

        std::size_t acc = 0;
        for (std::size_t i = 0; i < foundProps.size(); ++i)
        {
            auto const requiredProps
                    = detail::required_prop_mask_for<descriptor>[i];

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
    requires(detail::versioned_decoder_enabled(layout_descriptor_for_v<T>))
class basic_decoder<T, Stream>
{
public:
    auto operator()(Stream &inStream, T &dest) const -> result<void>
    {
        DPLX_TRY(
                auto &&headInfo,
                dp::parse_object_head<Stream, layout_descriptor_for_v<T>.version
                                                      != null_def_version>(
                        inStream));

        if constexpr (layout_descriptor_for_v<T>.version != null_def_version)
        {
            if (layout_descriptor_for_v<T>.version != headInfo.version)
            {
                return errc::item_version_mismatch;
            }
        }

        return dp::decode_object_properties<layout_descriptor_for_v<T>, T,
                                            Stream>(inStream, dest,
                                                    headInfo.num_properties);
    }
};

} // namespace dplx::dp
