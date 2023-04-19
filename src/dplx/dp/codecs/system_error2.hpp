
// Copyright 2023 Henrik Steffen Gaßmann
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <concepts>

#include <status-code/status_code_domain.hpp>

#include <dplx/dp/fwd.hpp>

namespace dplx::dp
{

template <>
class codec<SYSTEM_ERROR2_NAMESPACE::status_code_domain::string_ref>
{
    using value_type = SYSTEM_ERROR2_NAMESPACE::status_code_domain::string_ref;

public:
    static auto size_of(emit_context &ctx, value_type const &value) noexcept
            -> std::uint64_t;
    static auto encode(emit_context &ctx, value_type const &value) noexcept
            -> result<void>;
};
template <>
class codec<SYSTEM_ERROR2_NAMESPACE::status_code_domain::
                    atomic_refcounted_string_ref>
    : public codec<SYSTEM_ERROR2_NAMESPACE::status_code_domain::string_ref>
{
};
template <std::derived_from<
        SYSTEM_ERROR2_NAMESPACE::status_code_domain::string_ref> StringRef>
class codec<StringRef>
    : public codec<SYSTEM_ERROR2_NAMESPACE::status_code_domain::string_ref>
{
};

} // namespace dplx::dp
