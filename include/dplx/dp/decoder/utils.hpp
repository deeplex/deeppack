
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp::detail
{

template <typename T, input_stream Stream>
struct mp_decode_value_fn
{
    Stream &inStream;
    T &dest;

    template <typename PropDefType>
    inline auto operator()(PropDefType const &propertyDef) -> result<void>
    {
        using property_decoder
                = dp::basic_decoder<typename PropDefType::value_type, Stream>;

        DPLX_TRY(property_decoder()(inStream, propertyDef.access(dest)));
        return oc::success();
    }
};

} // namespace dplx::dp::detail
