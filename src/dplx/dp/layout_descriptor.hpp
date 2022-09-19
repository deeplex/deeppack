
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/cncr/type_utils.hpp>

#include <dplx/dp/object_def.hpp>
#include <dplx/dp/tag_invoke.hpp>
#include <dplx/dp/tuple_def.hpp>

namespace dplx::dp
{

template <typename T>
concept exposed_static_layout_descriptor = requires
{
    T::layout_descriptor;
};

inline constexpr struct layout_descriptor_for_fn
{
    template <typename T>
        requires tag_invocable<layout_descriptor_for_fn, std::type_identity<T>>
    constexpr auto operator()(std::type_identity<T> tv) const noexcept
            -> tag_invoke_result_t<layout_descriptor_for_fn,
                                   std::type_identity<T>>
    {
        return cpo::tag_invoke(*this, tv);
    }

    template <typename T>
        requires(
                exposed_static_layout_descriptor<
                        T> && (is_object_def_v<cncr::remove_cref_t<decltype(T::layout_descriptor)>> || is_tuple_def_v<cncr::remove_cref_t<decltype(T::layout_descriptor)>>))
    friend constexpr decltype(auto) tag_invoke(layout_descriptor_for_fn,
                                               std::type_identity<T>) noexcept
    {
        return (T::layout_descriptor);
    }

} layout_descriptor_for;

template <typename T>
concept packable
        = !detail::is_type_identity<T>::value // tis a workaround for infinite
                                              // compiler recursion during
                                              // encoded_size_of_fn constraint
                                              // checks (for gcc 11.1)
       && tag_invocable<layout_descriptor_for_fn, std::type_identity<T>>;

template <typename T>
concept packable_object = packable<T> && is_object_def_v<cncr::remove_cref_t<
        tag_invoke_result_t<layout_descriptor_for_fn, std::type_identity<T>>>>;

template <typename T>
concept packable_tuple = packable<T> && is_tuple_def_v<cncr::remove_cref_t<
        tag_invoke_result_t<layout_descriptor_for_fn, std::type_identity<T>>>>;

inline constexpr std::uint32_t null_def_version = 0xffff'ffffu;

template <typename T>
    requires packable<T>
inline constexpr auto layout_descriptor_for_v
        = layout_descriptor_for(std::type_identity<T>{});

} // namespace dplx::dp

namespace dplx::dp::detail
{

template <typename T>
constexpr auto versioned_decoder_enabled(T const &descriptor) noexcept -> bool
{
    return descriptor.allow_versioned_auto_decoder
        || descriptor.version == null_def_version;
}

} // namespace dplx::dp::detail
