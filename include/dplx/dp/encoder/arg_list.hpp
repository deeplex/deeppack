
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/detail/mp_lite.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/stream.hpp>
#include <dplx/dp/type_encoder.hpp>

namespace dplx::dp::detail
{

template <output_stream Stream, typename T>
class arg_list_encoder;

template <output_stream Stream, typename... TArgs>
class arg_list_encoder<Stream, mp_list<TArgs...>>
{
    Stream *mOutStream;

public:
    explicit inline arg_list_encoder(Stream &outStream) noexcept
        : mOutStream(&outStream)
    {
    }

    static inline auto encode(Stream &outStream,
                              select_proper_param_type<TArgs>... values)
        -> result<void>
    {
        DPLX_TRY(type_encoder<Stream>::array(outStream, sizeof...(TArgs)));

        result<void> rx = success();

        [[maybe_unused]] bool failed =
            (... || (rx = basic_encoder<Stream, TArgs>()(outStream, values))
                        .has_failure());

        return rx;
    }

    auto inline operator()(select_proper_param_type<TArgs>... values) const
        -> result<void>
    {
        return encode(*mOutStream, values...);
    }
};

} // namespace dplx::dp::detail