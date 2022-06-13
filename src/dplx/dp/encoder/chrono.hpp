
// Copyright Henrik Steffen Gaßmann 2021.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <chrono>

#include <dplx/dp/encoder/api.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/item_emitter.hpp>

namespace dplx::dp
{

template <typename Rep, typename Period, output_stream Stream>
class basic_encoder<std::chrono::duration<Rep, Period>, Stream>
{
    using emit = item_emitter<Stream>;

public:
    using value_type = std::chrono::duration<Rep, Period>;

    auto operator()(Stream &outStream, value_type value) -> result<void>
    {
        return emit::integer(outStream, value.count());
    }
};

template <typename Rep, typename Period>
constexpr auto tag_invoke(encoded_size_of_fn,
                          std::chrono::duration<Rep, Period> value) -> unsigned
{
    if constexpr (std::is_signed_v<Rep>)
    {
        auto const count = value.count();
        using uvalue_type = std::make_unsigned_t<Rep>;
        auto const signmask = static_cast<uvalue_type>(
                count >> (detail::digits_v<uvalue_type> - 1));
        // complement negatives
        auto const uvalue = signmask ^ static_cast<uvalue_type>(count);

        return dp::additional_information_size(uvalue);
    }
    else
    {
        return dp::additional_information_size(value.count());
    }
}

} // namespace dplx::dp
