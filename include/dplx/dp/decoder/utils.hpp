
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/mp11/integral.hpp>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp::detail
{

template <auto const &descriptor, typename T, input_stream Stream>
struct mp_decode_value_fn
{
    Stream &inStream;
    T &dest;

    template <std::size_t I>
    auto operator()(boost::mp11::mp_size_t<I>) -> result<std::size_t>
    {
        constexpr decltype(auto) propertyDef =
            descriptor.template property<I>();

        using property_decoder =
            dp::basic_decoder<typename decltype(propertyDef.decl_value())::type,
                              Stream>;

        DPLX_TRY(property_decoder()(inStream, propertyDef.access(dest)));
        return I;
    }
};

} // namespace dplx::dp::detail
