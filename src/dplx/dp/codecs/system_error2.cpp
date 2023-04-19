
// Copyright 2023 Henrik Steffen Gaßmann
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/system_error2.hpp"

#include <dplx/dp/api.hpp>
#include <dplx/dp/items/emit_core.hpp>
#include <dplx/dp/items/item_size_of_core.hpp>

namespace dplx::dp
{

auto codec<SYSTEM_ERROR2_NAMESPACE::status_code_domain::string_ref>::size_of(
        emit_context &ctx, value_type const &value) noexcept -> std::uint64_t
{
    return dp::item_size_of_u8string(ctx, value.size());
}
auto codec<SYSTEM_ERROR2_NAMESPACE::status_code_domain::string_ref>::encode(
        emit_context &ctx, value_type const &value) noexcept -> result<void>
{
    return dp::emit_u8string(ctx, value.data(), value.size());
}

} // namespace dplx::dp
