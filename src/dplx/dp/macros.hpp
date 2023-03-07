
// Copyright Henrik Steffen Gaßmann 2021.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/dp/fwd.hpp>

// NOLINTBEGIN(modernize-macro-to-enum)
// NOLINTBEGIN(cppcoreguidelines-macro-usage)

// NOLINTBEGIN(bugprone-macro-parentheses)
#define DPLX_DP_DECLARE_CODEC_SIMPLE(_fq_type)                                 \
    template <>                                                                \
    class dplx::dp::codec<_fq_type>                                            \
    {                                                                          \
        using value_type = _fq_type;                                           \
                                                                               \
    public:                                                                    \
        static auto size_of(emit_context &ctx, _fq_type const &value) noexcept \
                -> std::uint64_t;                                              \
        static auto encode(emit_context &ctx, _fq_type const &value) noexcept  \
                -> result<void>;                                               \
        static auto decode(parse_context &ctx, _fq_type &outValue) noexcept    \
                -> result<void>;                                               \
    }
// NOLINTEND(bugprone-macro-parentheses)

// NOLINTEND(cppcoreguidelines-macro-usage)
// NOLINTEND(modernize-macro-to-enum)
