
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <type_traits>

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp
{

// the decode APIs are not meant to participate in ADL and are therefore
// niebloids
inline constexpr struct decode_fn final
{
    template <typename T, input_stream Stream>
    requires decodable<T, Stream> inline auto operator()(Stream &inStream,
                                                         T &dest) const
            -> result<void>
    {
        DPLX_TRY((basic_decoder<T, Stream>()(inStream, dest)));
        return success();
    }

    template <typename T, input_stream Stream>
    requires(decodable<T, Stream> &&std::is_default_constructible_v<T>
                     &&std::is_move_constructible_v<T>) inline auto
    operator()(as_value_t<T>, Stream &inStream) const -> result<T>
    {
        auto value = T();
        DPLX_TRY((basic_decoder<T, Stream>()(inStream, value)));
        return dp::success(std::move(value));
    }

} decode{};

} // namespace dplx::dp
