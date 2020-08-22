
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <compare>
#include <type_traits>

#include <dplx/dp/detail/type_utils.hpp>

namespace dplx::dp
{

template <auto M>
requires std::is_member_object_pointer_v<decltype(M)> struct tuple_member_def
{
private:
    using member_object_pointer_type_traits =
        detail::member_object_pointer_type_traits<decltype(M)>;

public:
    using value_type = typename member_object_pointer_type_traits::value_type;
    using class_type = std::remove_cvref_t<
        typename member_object_pointer_type_traits::class_type>;

    int nothing = 0;

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

    constexpr auto operator==(tuple_member_def const &) const noexcept
        -> bool = default;
    constexpr auto operator<=>(tuple_member_def const &) const noexcept
        -> std::strong_ordering = default;
};

template <auto... Properties>
struct tuple_def
{
    using class_type = std::common_type_t<
        typename std::remove_cvref_t<decltype(Properties)>::class_type...>;

    static constexpr std::size_t num_properties = sizeof...(Properties);

    std::uint32_t version = 0xffff'ffff;
    bool allow_versioned_auto_decoder = false;

    template <std::size_t N>
    static constexpr decltype(auto) property() noexcept
    {
        static_assert(N < num_properties);
        return detail::nth_param_v<N, Properties...>;
    }

    constexpr auto operator==(tuple_def const &) const noexcept
        -> bool = default;
    constexpr auto operator<=>(tuple_def const &) const noexcept
        -> std::strong_ordering = default;
};

template <typename>
struct is_tuple_def : std::false_type
{
};
template <auto... Properties>
struct is_tuple_def<tuple_def<Properties...>> : std::true_type
{
};

template <typename T>
inline constexpr bool is_tuple_def_v = is_tuple_def<T>::value;

} // namespace dplx::dp
