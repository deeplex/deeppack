
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstdint>

#include <boost/mp11/algorithm.hpp>

#include <dplx/dp/api.hpp>
#include <dplx/dp/cpos/property_id_hash.hpp>
#include <dplx/dp/detail/perfect_hash.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/emit_core.hpp>
#include <dplx/dp/items/encoded_item_head_size.hpp>
#include <dplx/dp/items/item_size_of_core.hpp>
#include <dplx/dp/items/parse_context.hpp>
#include <dplx/dp/items/parse_core.hpp>
#include <dplx/dp/layout_descriptor.hpp>
#include <dplx/dp/object_def.hpp>

namespace dplx::dp::detail
{

template <typename T>
struct mp_encode_object_property_fn
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    emit_context &ctx;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
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

template <typename T>
struct mp_size_of_object_property_fn
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    emit_context &ctx;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    T const &value;

    template <typename PropDefType>
    constexpr auto operator()(PropDefType const &propertyDef) const noexcept
            -> std::uint64_t
    {
        using key_type = typename PropDefType::id_type;
        using value_type = typename PropDefType::value_type;

        std::uint64_t const keySize
                = codec<key_type>::size_of(ctx, propertyDef.id);
        std::uint64_t const valueSize = codec<value_type>::size_of(
                ctx,
                static_cast<value_type const &>(propertyDef.access(value)));
        return keySize + valueSize;
    }
};

} // namespace dplx::dp::detail

namespace dplx::dp
{

template <auto const &descriptor>
inline auto
encode_object(emit_context &ctx,
              detail::descriptor_class_type<descriptor> const &value) noexcept
        -> result<void>
{
    using encode_property_fn = detail::mp_encode_object_property_fn<
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
inline auto encode_object(emit_context &ctx, T const &value) noexcept
        -> result<void>
{
    return dp::encode_object<layout_descriptor_for_v<T>>(ctx, value);
}

template <auto const &descriptor>
constexpr auto
size_of_object(emit_context &ctx,
               detail::descriptor_class_type<descriptor> const &value) noexcept
        -> std::uint64_t
{
    using size_of_property_fn = detail::mp_size_of_object_property_fn<
            detail::descriptor_class_type<descriptor>>;

    std::uint64_t prefixSize = 0U;
    if constexpr (descriptor.version == null_def_version)
    {
        prefixSize += dp::encoded_item_head_size<type_code::map>(
                descriptor.num_properties);
    }
    else
    {
        prefixSize += dp::encoded_item_head_size<type_code::map>(
                descriptor.num_properties + 1U);

        prefixSize += dp::item_size_of_integer(ctx, 0U);
        prefixSize += dp::item_size_of_integer(ctx, descriptor.version);
    }

    return prefixSize
           + descriptor.mp_map_fold_left(size_of_property_fn{ctx, value});
}

template <packable_object T>
constexpr auto size_of_object(emit_context &ctx, T const &value) noexcept
        -> std::uint64_t
{
    return dp::size_of_object<layout_descriptor_for_v<T>>(ctx, value);
}

} // namespace dplx::dp

// property_id_lookup
namespace dplx::dp::detail
{

inline constexpr std::size_t unknown_property_id = ~static_cast<std::size_t>(0);

template <typename IdType, std::size_t NumIds, bool use_perfect_hash>
struct property_id_lookup_fn;

template <typename IdType, std::size_t NumIds>
struct property_id_lookup_fn<IdType, NumIds, true>
{
private:
    using id_type = IdType;
    using array_type = std::array<id_type, NumIds>;

    std::span<id_type const, NumIds> ids;
    perfect_hasher<id_type, NumIds, property_id_hash_fn> hash;

public:
    constexpr property_id_lookup_fn(std::span<id_type const, NumIds> idsInit)
        : ids(idsInit)
        , hash(ids)
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

    std::span<id_type const, NumIds> ids;

public:
    constexpr property_id_lookup_fn(std::span<id_type const, NumIds> idsInit)
        : ids(idsInit)
    {
    }

    template <typename TLike>
    constexpr auto operator()(TLike &&id) const noexcept -> std::size_t
    {
        constexpr std::size_t linear_search_efficiency_threshold = 64U;

        auto const *const begin = ids.data();
        auto const *const end = ids.data() + ids.size();
        id_type const *it; // NOLINT(cppcoreguidelines-init-variables)
        if constexpr (NumIds <= linear_search_efficiency_threshold)
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

} // namespace dplx::dp::detail

// decode_object_property
namespace dplx::dp::detail
{

template <auto const &descriptor, std::size_t Offset = 0U>
struct decode_prop_by_index_fn
{
    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    parse_context &ctx;
    typename cncr::remove_cref_t<decltype(descriptor)>::class_type &self;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

    template <std::size_t I>
    inline auto operator()(boost::mp11::mp_size_t<I>) const noexcept
            -> result<std::size_t>
    {
        constexpr auto &propDef = descriptor.template property<I + Offset>();
        DPLX_TRY(dp::decode(ctx, propDef.access(self)));
        return I + Offset;
    }
};

template <auto const &descriptor>
class decode_object_property_fn
{
#if DPLX_DP_WORKAROUND(DPLX_COMP_GNUC, <=, 10, 1, 0)
    // fixed_u8string id comparison operator is borked
#else
    static_assert(std::is_sorted(descriptor.ids.begin(), descriptor.ids.end()));
#endif

    using odef_type = cncr::remove_cref_t<decltype(descriptor)>;
    using id_type = typename odef_type::id_type;
    using id_runtime_type = typename odef_type::id_runtime_type;
    using class_type = typename odef_type::class_type;

    using lookup_fn
            = property_id_lookup_fn<id_type, descriptor.ids.size(), false>;

public:
    auto operator()(parse_context &ctx, class_type &dest) const
            -> result<std::size_t>
    {
        DPLX_TRY(auto &&id, decode(as_value<id_runtime_type>, ctx));

        auto const idx = lookup_fn{descriptor.ids}(id);
        if (idx == unknown_property_id)
        {
            return errc::unknown_property;
        }

        return boost::mp11::mp_with_index<descriptor.ids.size()>(
                idx, decode_prop_by_index_fn<descriptor>{ctx, dest});
    }
};

template <auto const &descriptor>
inline constexpr decode_object_property_fn<descriptor> decode_object_property{};

template <typename T>
consteval auto index_of_limit(T const *elems,
                              std::size_t const num,
                              T const limit) noexcept -> std::size_t
{
    for (std::size_t i = 0; i < num; ++i)
    {
        // consteval -> misuse would be catched by the compiler
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (elems[i] >= limit)
        {
            return i;
        }
    }
    return num;
}

//*
template <auto const &descriptor>
    requires cncr::unsigned_integer<
            typename cncr::remove_cref_t<decltype(descriptor)>::id_type>
class decode_object_property_fn<descriptor>
{
    using descriptor_type = cncr::remove_cref_t<decltype(descriptor)>;
    using id_type = typename descriptor_type::id_type;
    using class_type =
            typename cncr::remove_cref_t<decltype(descriptor)>::class_type;

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

    static constexpr property_id_lookup_fn<id_type, id_map_size, false> lookup{
            std::span<id_type const, descriptor.ids.size()>(descriptor.ids)
                    .template subspan<small_ids_end>()};

    struct decode_prop_small_id_fn
    {
        // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
        parse_context &ctx;
        class_type &self;
        // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

        template <std::size_t I>
        inline auto operator()(boost::mp11::mp_size_t<I>) const noexcept
                -> result<std::size_t>
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
                DPLX_TRY(dp::decode(ctx, propertyDef.access(self)));
                return propPos;
            }
        }
    };
    using decode_prop_large_small_fn
            = decode_prop_by_index_fn<descriptor, small_ids_end>;

public:
    auto operator()(parse_context &ctx, class_type &dest) const
            -> result<std::size_t>
    {
        id_type id; // NOLINT(cppcoreguidelines-pro-type-member-init)
        DPLX_TRY(dp::parse_integer<id_type>(ctx, id));

        if constexpr (small_ids_end != 0)
        {
            if (id < small_id_limit)
            {
                return boost::mp11::mp_with_index<small_id_limit>(
                        static_cast<std::size_t>(id),
                        decode_prop_small_id_fn{ctx, dest});
            }
        }
        if constexpr (small_ids_end != descriptor.ids.size())
        {
            if (auto const idx = lookup(id); idx != unknown_property_id)
            {
                return boost::mp11::mp_with_index<id_map_size>(
                        idx, decode_prop_large_small_fn{ctx, dest});
            }
        }
        return errc::unknown_property;
    }
};
// */

} // namespace dplx::dp::detail

// required_prop_mask_for
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
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (std::size_t shift = 0, offset = 0; it != end; ++it, ++shift)
    {
        if (shift == digits)
        {
            shift = 0;
            offset += 1;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        buckets[offset] |= static_cast<std::size_t>(*it) << shift;
    }
    return buckets;
}

template <template <auto...> typename ObjectDefLike, auto... Properties>
constexpr auto
compress_required_props(ObjectDefLike<Properties...> const &) noexcept
{
    return detail::compress_bitset<sizeof...(Properties)>(
            {Properties.required...});
}

template <auto const &descriptor>
inline constexpr auto required_prop_mask_for
        = detail::compress_required_props(descriptor);

} // namespace dplx::dp::detail

namespace dplx::dp
{

struct object_head_info
{
    std::int32_t num_properties;
    version_type version;

    friend constexpr auto operator==(object_head_info const &,
                                     object_head_info const &) noexcept -> bool
            = default;
};

template <bool isVersioned = true>
inline auto decode_object_head(parse_context &ctx,
                               std::bool_constant<isVersioned> = {})
        -> result<object_head_info>
{
    auto &&parseHeadRx = dp::parse_item_head(ctx);
    if (parseHeadRx.has_error()) [[unlikely]]
    {
        return static_cast<decltype(parseHeadRx) &&>(parseHeadRx)
                .assume_error();
    }
    item_head const &mapInfo = parseHeadRx.assume_value();
    if (mapInfo.type != type_code::map || mapInfo.indefinite())
    {
        return errc::item_type_mismatch;
    }
    if (mapInfo.value == 0)
    {
        return object_head_info{0, null_def_version};
    }

    // every prop consists of two items each being at least 1B big
    if (mapInfo.value > (ctx.in.input_size() / 2))
    {
        return errc::end_of_stream;
    }
    if (mapInfo.value >= static_cast<std::uint64_t>(
                std::numeric_limits<std::int32_t>::max() / 2))
    {
        return errc::too_many_properties;
    }
    auto const numProps = static_cast<std::int32_t>(mapInfo.value);

    if constexpr (!isVersioned)
    {
        return object_head_info{numProps, null_def_version};
    }
    else
    {
        // the version property id is posint 0
        // and always encoded as a single byte
        DPLX_TRY(ctx.in.require_input(1U));
        if (*ctx.in.data() != std::byte{})
        {
            return object_head_info{numProps, null_def_version};
        }
        ctx.in.discard_buffered(1U);

        // 0xffff'ffff => max() is reserved as null_def_version
        std::uint32_t version; // NOLINT(cppcoreguidelines-init-variables)
        DPLX_TRY(dp::parse_integer(ctx, version, null_def_version - 1U));

        return object_head_info{numProps - 1, version};
    }
}

template <auto const &descriptor>
inline auto decode_object_properties(
        parse_context &ctx,
        typename cncr::remove_cref_t<decltype(descriptor)>::class_type &dest,
        std::int32_t numProperties) -> result<void>
{
    if constexpr (descriptor.has_optional_properties)
    {
        std::array<std::size_t,
                   detail::required_prop_mask_for<descriptor>.size()>
                foundProps{};

        for (std::int32_t i = 0; i < numProperties; ++i)
        {
            DPLX_TRY(auto &&which,
                     detail::decode_object_property<descriptor>(ctx, dest));

            auto const offset = which / detail::digits_v<std::size_t>;
            auto const shift = which % detail::digits_v<std::size_t>;

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            foundProps[offset] |= static_cast<std::size_t>(1) << shift;
        }

        std::size_t acc = 0;
        for (std::size_t i = 0; i < foundProps.size(); ++i)
        {
            auto const requiredProps
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                    = detail::required_prop_mask_for<descriptor>[i];

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
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
            DPLX_TRY(detail::decode_object_property<descriptor>(ctx, dest));
        }
    }
    return success();
}

template <packable_object T>
inline auto decode_object(parse_context &ctx, T &value) -> result<void>
{
    DPLX_TRY(auto &&headInfo,
             dp::decode_object_head<layout_descriptor_for_v<T>.version
                                    != null_def_version>(ctx));

    if constexpr (layout_descriptor_for_v<T>.version != null_def_version)
    {
        if (layout_descriptor_for_v<T>.version != headInfo.version)
        {
            return errc::item_version_mismatch;
        }
    }

    return dp::decode_object_properties<layout_descriptor_for_v<T>>(
            ctx, value, headInfo.num_properties);
}

} // namespace dplx::dp
