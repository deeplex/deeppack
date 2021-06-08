
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>

#include <algorithm>
#include <array>
#include <compare>
#include <concepts>
#include <string_view>
#include <type_traits>

#include <boost/predef/compiler.h>
#include <boost/predef/other/workaround.h>

#if BOOST_PREDEF_WORKAROUND(BOOST_COMP_GNUC, <=, 10, 2, 0)
// gcc has a problem with the defaulted <=> over structs containing arrays
// therefore we need to use std::u8string as runtime string type
#include <string>
#endif

#include <dplx/dp/customization.hpp>
#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/tag_invoke.hpp>
#include <dplx/dp/type_code.hpp>

namespace dplx::dp
{

template <std::size_t N>
struct fixed_u8string
{
    unsigned int mNumCodeUnits;
    char8_t mCodeUnits[N];

    constexpr fixed_u8string() noexcept
        : mNumCodeUnits{0}
        , mCodeUnits{}
    {
    }
    constexpr fixed_u8string(char8_t const (&codeUnits)[N + 1]) noexcept
        : mNumCodeUnits{N}
    {
        std::copy_n(codeUnits, N, mCodeUnits);
    }
    template <std::size_t N2>
    constexpr fixed_u8string(fixed_u8string<N2> const &other) noexcept
        : mNumCodeUnits{static_cast<unsigned int>(other.size())}
        , mCodeUnits{}
    {
        static_assert(N >= N2);
        std::copy_n(other.data(), other.size(), mCodeUnits);
    }

    constexpr fixed_u8string(fixed_u8string const &) noexcept = default;
    constexpr fixed_u8string(fixed_u8string &&) noexcept = default;
    constexpr auto operator=(fixed_u8string const &) noexcept
            -> fixed_u8string & = default;
    constexpr auto operator=(fixed_u8string &&) noexcept
            -> fixed_u8string & = default;

    friend inline constexpr auto operator==(fixed_u8string const &,
                                            fixed_u8string const &) noexcept
            -> bool
            = default;
    friend inline constexpr auto operator<=>(fixed_u8string const &,
                                             fixed_u8string const &) noexcept
            -> std::strong_ordering = default;

    friend inline constexpr auto operator==(fixed_u8string const &lhs,
                                            std::u8string_view rhs) noexcept
            -> bool
    {
        return std::u8string_view(lhs) == rhs;
    }
#if BOOST_PREDEF_WORKAROUND(BOOST_COMP_CLANG, <, 13, 0, 0)

    friend inline constexpr auto operator<=>(fixed_u8string const &lhs,
                                             std::u8string_view rhs) noexcept
            -> std::strong_ordering
    {
        std::u8string_view view(lhs);
        return std::lexicographical_compare_three_way(view.begin(), view.end(),
                                                      rhs.begin(), rhs.end());
    }

#else

    friend inline constexpr auto operator<=>(fixed_u8string const &lhs,
                                             std::u8string_view rhs) noexcept
            -> std::strong_ordering
    {
        return std::u8string_view(lhs) <=> rhs;
    }

#endif

    constexpr operator std::u8string_view() const noexcept
    {
        return std::u8string_view(mCodeUnits, mNumCodeUnits);
    }

    constexpr auto data() noexcept -> char8_t *
    {
        return mCodeUnits;
    }
    constexpr auto data() const noexcept -> char8_t const *
    {
        return mCodeUnits;
    }
    constexpr auto size() const noexcept -> std::size_t
    {
        return mNumCodeUnits;
    }
    static constexpr auto max_size() noexcept -> std::size_t
    {
        return N;
    }

    friend inline auto tag_invoke(container_reserve_fn,
                                  fixed_u8string &self,
                                  std::size_t const capacity) noexcept
            -> result<void>
    {
        if (capacity <= self.size())
        {
            return oc::success();
        }
        return errc::not_enough_memory;
    }
    friend inline auto tag_invoke(container_resize_fn,
                                  fixed_u8string &self,
                                  std::size_t const newSize) noexcept
            -> result<void>
    {
        if (newSize <= self.max_size())
        {
            if (newSize > self.size())
            {
                std::fill(self.mCodeUnits + self.mNumCodeUnits,
                          self.mCodeUnits + newSize, char8_t{});
            }

            self.mNumCodeUnits = static_cast<unsigned int>(newSize);
            return oc::success();
        }
        return errc::not_enough_memory;
    }
    friend inline auto tag_invoke(container_resize_for_overwrite_fn,
                                  fixed_u8string &self,
                                  std::size_t const newSize) noexcept
            -> result<void>
    {
        if (newSize <= self.max_size())
        {
            self.mNumCodeUnits = static_cast<unsigned int>(newSize);
            return oc::success();
        }
        return errc::not_enough_memory;
    }
};

template <std::size_t N>
fixed_u8string(char8_t const (&)[N]) -> fixed_u8string<N - 1>;

} // namespace dplx::dp

namespace std
{

template <std::size_t N>
struct common_type<dplx::dp::fixed_u8string<N>, dplx::dp::fixed_u8string<N>>
{
    using type = dplx::dp::fixed_u8string<N>;
};

template <std::size_t N1, std::size_t N2>
struct common_type<dplx::dp::fixed_u8string<N1>, dplx::dp::fixed_u8string<N2>>
{
    using type = dplx::dp::fixed_u8string<
            dplx::dp::detail::div_ceil(N1 < N2 ? N2 : N1, 16u) * 16u>;
};

} // namespace std

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
    using id_type = decltype(Id);
    using id_runtime_type = IdRuntimeType;
    using class_type = std::remove_cvref_t<
            typename member_object_pointer_type_traits::class_type>;
    using value_type = std::remove_cvref_t<decltype((
            (std::declval<class_type &>().*M).*....*Ms))>;

    static constexpr id_type const &id = Id;
    bool required = true;

    static auto decl_value() noexcept
            -> std::type_identity<std::remove_cvref_t<value_type>>;
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

#if BOOST_PREDEF_WORKAROUND(BOOST_COMP_GNUC, <=, 10, 2, 0)

template <fixed_u8string Id, auto M, auto... Ms>
using named_property_def = basic_property_def<std::u8string, Id, M, Ms...>;

#else

template <fixed_u8string Id, auto M, auto... Ms>
using named_property_def = basic_property_def<decltype(Id), Id, M, Ms...>;

#endif

template <auto Id, typename AccessorType, typename IdRuntimeType = decltype(Id)>
struct basic_property_fun
{
public:
    using id_type = decltype(Id);
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
using property_fun = basic_property_fun<Id, AccessorType>;

#if BOOST_PREDEF_WORKAROUND(BOOST_COMP_GNUC, <=, 10, 2, 0)

template <fixed_u8string Id, typename AccessorType>
using named_property_fun = basic_property_fun<Id, AccessorType, std::u8string>;

#else

template <fixed_u8string Id, typename AccessorType>
using named_property_fun = basic_property_fun<Id, AccessorType, decltype(Id)>;

#endif

template <auto... Properties>
struct object_def
{
    using id_type = std::common_type_t<
            typename std::remove_cvref_t<decltype(Properties)>::id_type...>;
    using id_runtime_type = std::common_type_t<typename std::remove_cvref_t<
            decltype(Properties)>::id_runtime_type...>;
    using class_type = detail::contravariance_t<
            typename std::remove_cvref_t<decltype(Properties)>::class_type...>;

    static constexpr std::size_t num_properties = sizeof...(Properties);
    static constexpr bool has_optional_properties
            = !(... && Properties.required);
    static constexpr std::array<id_type, num_properties> ids{Properties.id...};

    std::uint32_t version = 0xffff'ffff;
    bool allow_versioned_auto_decoder = false;

    constexpr object_def() noexcept = default;

    template <std::size_t N>
    static constexpr decltype(auto) property() noexcept
    {
        static_assert(N < num_properties);
        return detail::nth_param_v<N, Properties...>;
    }

    template <typename MapFn>
    static constexpr auto mp_map_fold_left(MapFn &&map)
    {
        return (... + static_cast<MapFn &&>(map)(Properties));
    }
};

template <typename>
struct is_object_def : std::false_type
{
};
template <auto... Properties>
struct is_object_def<object_def<Properties...>> : std::true_type
{
};

template <typename T>
concept is_object_def_v = is_object_def<T>::value;

} // namespace dplx::dp
