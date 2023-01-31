
// Copyright Henrik Steffen Gaßmann 2020, 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <dplx/dp/fwd.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/streams/output_buffer.hpp>

namespace dp_tests
{
struct simple_encodable
{
    unsigned char value;

    auto operator++() noexcept -> simple_encodable &
    {
        ++value;
        return *this;
    }
};
} // namespace dp_tests

template <>
class dplx::dp::codec<dp_tests::simple_encodable>
{
public:
    static auto size_of(emit_context const &,
                        dp_tests::simple_encodable) noexcept -> std::uint64_t
    {
        return 1U;
    }
    static auto encode(emit_context const &ctx,
                       dp_tests::simple_encodable obj) noexcept -> result<void>
    {
        DPLX_TRY(ctx.out.ensure_size(1U));
        *ctx.out.data() = static_cast<std::byte>(obj.value);
        ctx.out.commit_written(1U);
        return oc::success();
    }
};
