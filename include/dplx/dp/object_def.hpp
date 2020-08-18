
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

#if BOOST_PREDEF_WORKAROUND(BOOST_COMP_GNUC, <=, 10, 1, 0)
// gcc has a problem with the defaulted <=> over structs containing arrays
// therefore we need to use std::u8string as runtime string type
#include <string>
#endif

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

    constexpr auto operator==(fixed_u8string const &) const noexcept
        -> bool = default;
    constexpr auto operator<=>(fixed_u8string const &) const noexcept
        -> std::strong_ordering = default;

    constexpr auto operator==(std::u8string_view str) const noexcept -> bool
    {
        return str == std::u8string_view(*this);
    }
    constexpr auto operator<=>(std::u8string_view str) const noexcept
        -> std::strong_ordering
    {
        return str <=> std::u8string_view(*this);
    }

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

template <auto Id, auto M, typename IdRuntimeType = decltype(Id)>
requires std::is_member_object_pointer_v<decltype(M)> struct basic_property_def
{
private:
    using member_object_pointer_type_traits =
        detail::member_object_pointer_type_traits<decltype(M)>;

public:
    using id_type = decltype(Id);
    using id_runtime_type = IdRuntimeType;
    using value_type = typename member_object_pointer_type_traits::value_type;
    using class_type = std::remove_cvref_t<
        typename member_object_pointer_type_traits::class_type>;

    static constexpr id_type const &id = Id;
    bool required = true;

    static auto decl_value() noexcept
        -> std::type_identity<std::remove_cvref_t<value_type>>;
    static auto decl_class() noexcept -> std::type_identity<class_type>;

    static inline auto access(class_type &v) noexcept -> value_type &
    {
        return v.*M;
    }
    static inline auto access(class_type const &v) noexcept
        -> value_type const &
    {
        return v.*M;
    }

    constexpr auto operator==(basic_property_def const &) const noexcept
        -> bool = default;
    constexpr auto operator<=>(basic_property_def const &) const noexcept
        -> std::strong_ordering = default;
};

template <std::uint32_t Id, auto M>
using property_def = basic_property_def<Id, M>;

#if BOOST_PREDEF_WORKAROUND(BOOST_COMP_GNUC, <=, 10, 1, 0)

template <fixed_u8string Id, auto M>
using named_property_def = basic_property_def<Id, M, std::u8string>;

#else

template <fixed_u8string Id, auto M>
using named_property_def = basic_property_def<Id, M, decltype(Id)>;

#endif

template <auto... Properties>
struct object_def
{
    using param_0_type =
        std::remove_cvref_t<decltype(detail::nth_param_v<0, Properties...>)>;
    using id_type = std::common_type_t<
        typename std::remove_cvref_t<decltype(Properties)>::id_type...>;
    using id_runtime_type = std::common_type_t<
        typename std::remove_cvref_t<decltype(Properties)>::id_runtime_type...>;
    using class_type = typename param_0_type::class_type;

    static constexpr std::size_t num_properties = sizeof...(Properties);
    static constexpr bool has_optional_properties =
        !(... && Properties.required);
    static constexpr std::array<id_type, num_properties> ids{Properties.id...};

    template <std::size_t N>
    static constexpr decltype(auto) property() noexcept
    {
        static_assert(N < num_properties);
        return detail::nth_param_v<N, Properties...>;
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
inline constexpr bool is_object_def_v = is_object_def<T>::value;

} // namespace dplx::dp
