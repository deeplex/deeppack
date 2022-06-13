
// Copyright Henrik Steffen Gaßmann 2021.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <filesystem>

#include <dplx/dp/encoder/api.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/item_emitter.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp
{

template <output_stream Stream>
class basic_encoder<std::filesystem::path, Stream>
{
    using emit = item_emitter<Stream>;

public:
    using value_type = std::filesystem::path;

    auto operator()(Stream &outStream, value_type const &value) -> result<void>
    {
        try
        {
            auto const generic = value.generic_u8string();
            auto const numCodeUnits = generic.size();
            DPLX_TRY(emit::u8string(outStream, numCodeUnits));

            DPLX_TRY(dp::write(
                    outStream,
                    reinterpret_cast<std::byte const *>(generic.data()),
                    numCodeUnits));
            return oc::success();
        }
        catch (std::bad_alloc const &)
        {
            return errc::not_enough_memory;
        }
    }
};

inline auto tag_invoke(encoded_size_of_fn, std::filesystem::path const &value)
        -> std::size_t
{
    auto const numCodeUnits = value.generic_u8string().size();
    return dp::additional_information_size(numCodeUnits) + numCodeUnits;
}

} // namespace dplx::dp
