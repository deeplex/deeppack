
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <algorithm>
#include <array>
#include <compare>
#include <concepts>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include <dplx/cncr/math_supplement.hpp>
#include <dplx/cncr/pack_utils.hpp>
#include <dplx/cncr/type_utils.hpp>
#include <dplx/predef/compiler.h>

#include <dplx/dp/detail/workaround.hpp>
#if DPLX_DP_WORKAROUND(DPLX_COMP_GNUC, <=, 10, 2, 0)
// gcc has a problem with the defaulted <=> over structs containing arrays
// therefore we need to use std::u8string as runtime string type
#include <string>
#endif

#include <dplx/dp/codecs/fixed_u8string.hpp>
#include <dplx/dp/customization.hpp>
#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/items/type_code.hpp>

namespace dplx::dp
{

template <typename IdRuntimeType, auto Id, auto M, auto... Ms>
    requires(std::is_member_object_pointer_v<decltype(M)> &&...
                     &&std::is_member_object_pointer_v<decltype(Ms)>)
struct basic_property_def
{
private:
    using member_object_pointer_type_traits
            = detail::member_object_pointer_type_traits<decltype(M)>;

public:
    using id_type = cncr::remove_cref_t<decltype(Id)>;
    using id_runtime_type = IdRuntimeType;
    using class_type = cncr::remove_cref_t<
            typename member_object_pointer_type_traits::class_type>;
    using value_type = cncr::remove_cref_t<decltype((
            (std::declval<class_type &>().*M).*....*Ms))>;

    static constexpr id_type const &id = Id;
    bool required = true;

    static auto decl_value() noexcept -> std::type_identity<value_type>;
    static auto decl_class() noexcept -> std::type_identity<class_type>;

    static inline auto access(class_type &v) noexcept -> value_type &
    {
        return ((v.*M).*....*Ms);
    }
    static inline auto access(class_type const &v) noexcept
            -> value_type const &
    {
        return ((v.*M).*....*Ms);
    }

    friend inline constexpr auto operator==(basic_property_def const &,
                                            basic_property_def const &) noexcept
            -> bool
            = default;
    friend inline constexpr auto
    operator<=>(basic_property_def const &, basic_property_def const &) noexcept
            -> std::strong_ordering = default;
};

template <std::uint32_t Id, auto M, auto... Ms>
using property_def = basic_property_def<std::uint32_t, Id, M, Ms...>;

#if DPLX_DP_WORKAROUND(DPLX_COMP_GNUC, <=, 10, 1, 0)

template <fixed_u8string Id, auto M, auto... Ms>
using named_property_def = basic_property_def<std::u8string, Id, M, Ms...>;

#else

template <fixed_u8string Id, auto M, auto... Ms>
using named_property_def
        = basic_property_def<cncr::remove_cref_t<decltype(Id)>, Id, M, Ms...>;

#endif

template <auto Id,
          typename AccessorType,
          typename IdRuntimeType = cncr::remove_cref_t<decltype(Id)>>
struct basic_property_fun
{
public:
    using id_type = cncr::remove_cref_t<decltype(Id)>;
    using id_runtime_type = IdRuntimeType;
    using value_type = typename AccessorType::value_type;
    using class_type = typename AccessorType::class_type;

    static constexpr id_type const &id = Id;
    bool required = true;

    static auto decl_value() noexcept -> std::type_identity<value_type>;
    static auto decl_class() noexcept -> std::type_identity<class_type>;

    static inline auto access(class_type &v) noexcept -> value_type &
    {
        return *AccessorType{}(v);
    }
    static inline auto access(class_type const &v) noexcept
            -> value_type const &
    {
        return *AccessorType{}(v);
    }

    friend inline constexpr auto operator==(basic_property_fun const &,
                                            basic_property_fun const &) noexcept
            -> bool
            = default;
    friend inline constexpr auto
    operator<=>(basic_property_fun const &, basic_property_fun const &) noexcept
            -> std::strong_ordering = default;
};

template <std::uint32_t Id, typename AccessorType>
using property_fun = basic_property_fun<Id, AccessorType, std::uint32_t>;

#if DPLX_DP_WORKAROUND(DPLX_COMP_GNUC, <=, 10, 1, 0)

template <fixed_u8string Id, typename AccessorType>
using named_property_fun = basic_property_fun<Id, AccessorType, std::u8string>;

#else

template <fixed_u8string Id, typename AccessorType>
using named_property_fun
        = basic_property_fun<Id,
                             AccessorType,
                             cncr::remove_cref_t<decltype(Id)>>;

#endif

template <auto... Properties>
struct object_def
{
    using id_type = std::common_type_t<
            typename cncr::remove_cref_t<decltype(Properties)>::id_type...>;
    using id_runtime_type = std::common_type_t<typename cncr::remove_cref_t<
            decltype(Properties)>::id_runtime_type...>;
    using class_type = detail::covariance_t<
            typename cncr::remove_cref_t<decltype(Properties)>::class_type...>;

    static constexpr std::size_t num_properties = sizeof...(Properties);
    static constexpr bool has_optional_properties
            = !(... && Properties.required);
    static constexpr std::array<id_type, num_properties> ids{Properties.id...};

    std::uint32_t version = null_def_version;
    bool allow_versioned_auto_decoder = false;

    template <std::size_t N>
    static constexpr auto property() noexcept -> decltype(auto)
    {
        static_assert(N < num_properties);
        return cncr::nth_param<N>(Properties...);
    }

    template <typename Fn>
    static inline auto mp_for_dots(Fn &&fn) -> result<void>
    {
        result<void> rx = oc::success();

        [[maybe_unused]] bool failed
                = (...
                   || detail::try_extract_failure(
                           static_cast<Fn &&>(fn)(Properties), rx));

        return rx;
    }

    template <typename MapFn>
    static constexpr auto mp_map_fold_left(MapFn &&map)
    {
        return (... + static_cast<MapFn &&>(map)(Properties));
    }

    friend inline constexpr auto operator==(object_def const &,
                                            object_def const &) noexcept -> bool
            = default;
    friend inline constexpr auto operator<=>(object_def const &,
                                             object_def const &) noexcept
            -> std::strong_ordering = default;
};

} // namespace dplx::dp
