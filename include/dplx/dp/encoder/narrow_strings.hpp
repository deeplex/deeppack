
// Copyright Henrik Steffen Gaßmann 2021.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/encoder/api.hpp>
#include <dplx/dp/item_emitter.hpp>

namespace dplx::dp
{

template <output_stream Stream>
class basic_encoder<std::string_view, Stream>
{
    using emit = item_emitter<Stream>;

public:
    using value_type = std::string_view;

    auto operator()(Stream &outStream, value_type value) const -> result<void>
    {
        auto const size = value.size();
        DPLX_TRY(emit::u8string(outStream, size));

        DPLX_TRY(dp::write(outStream,
                           reinterpret_cast<std::byte const *>(value.data()),
                           size));
        return success();
    }
};

template <output_stream Stream>
class basic_encoder<std::string, Stream>
    : public basic_encoder<std::string_view, Stream>
{
};

} // namespace dplx::dp
