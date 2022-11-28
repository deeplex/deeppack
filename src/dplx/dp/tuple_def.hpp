
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <compare>
#include <cstddef>
#include <type_traits>

#include <dplx/cncr/pack_utils.hpp>
#include <dplx/cncr/type_utils.hpp>

#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/disappointment.hpp>

namespace dplx::dp
{

template <auto M, auto... Ms>
    requires(std::is_member_object_pointer_v<decltype(M)> &&...
                     &&std::is_member_object_pointer_v<decltype(Ms)>)
struct tuple_member_def
{
private:
    using member_object_pointer_type_traits
            = detail::member_object_pointer_type_traits<decltype(M)>;

public:
    using class_type = cncr::remove_cref_t<
            typename member_object_pointer_type_traits::class_type>;
    using value_type = cncr::remove_cref_t<decltype((
            (std::declval<class_type &>().*M).*....*Ms))>;

    int nothing = 0;

    static auto decl_value() noexcept
            -> std::type_identity<cncr::remove_cref_t<value_type>>;
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

    friend inline constexpr auto operator==(tuple_member_def const &,
                                            tuple_member_def const &) noexcept
            -> bool
            = default;
    friend inline constexpr auto operator<=>(tuple_member_def const &,
                                             tuple_member_def const &) noexcept
            -> std::strong_ordering = default;
};

template <typename AccessorType>
struct tuple_member_fun
{
    using class_type = typename AccessorType::class_type;
    using value_type = typename AccessorType::value_type;

    static auto decl_value() noexcept -> std::type_identity<value_type>;
    static auto decl_class() noexcept -> std::type_identity<class_type>;

    int nothing = 0;

    static inline auto access(class_type &v) noexcept -> value_type &
    {
        // be lazy about this
        static_assert(member_accessor<AccessorType>);
        return *AccessorType{}(v);
    }
    static inline auto access(class_type const &v) noexcept
            -> value_type const &
    {
        // be lazy about this
        static_assert(member_accessor<AccessorType>);
        return *AccessorType{}(v);
    }

    friend inline constexpr auto operator==(tuple_member_fun const &,
                                            tuple_member_fun const &) noexcept
            -> bool
            = default;
    friend inline constexpr auto operator<=>(tuple_member_fun const &,
                                             tuple_member_fun const &) noexcept
            -> std::strong_ordering = default;
};

template <auto... Properties>
struct tuple_def
{
    using class_type = detail::covariance_t<
            typename cncr::remove_cref_t<decltype(Properties)>::class_type...>;

    static constexpr std::size_t num_properties = sizeof...(Properties);

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    std::uint32_t version = 0xffff'ffff;
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

    friend inline constexpr auto operator==(tuple_def const &,
                                            tuple_def const &) noexcept -> bool
            = default;
    friend inline constexpr auto operator<=>(tuple_def const &,
                                             tuple_def const &) noexcept
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
concept is_tuple_def_v = is_tuple_def<T>::value;

} // namespace dplx::dp
