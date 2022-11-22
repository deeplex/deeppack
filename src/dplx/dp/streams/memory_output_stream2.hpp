
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <span>

#include <dplx/dp/streams/output_buffer.hpp>

namespace dplx::dp
{

// the class is final and none of its base classes have public destructors
// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class memory_output_stream final : public output_buffer
{
    std::span<std::byte> mBuffer;

public:
    ~memory_output_stream() noexcept = default;
    explicit memory_output_stream(std::span<std::byte> output) noexcept
        : output_buffer(output.data(), output.size())
        , mBuffer(output)
    {
    }

    [[nodiscard]] auto written() const noexcept -> std::span<std::byte>
    {
        return mBuffer.first(mBuffer.size() - output_buffer::size());
    }

private:
    auto do_grow([[maybe_unused]] size_type const requestedSize) noexcept
            -> result<void> override
    {
        return errc::end_of_stream;
    }
    auto do_bulk_write([[maybe_unused]] std::byte const *const src,
                       [[maybe_unused]] std::size_t const size) noexcept
            -> result<void> override
    {
        return errc::end_of_stream;
    }
};

} // namespace dplx::dp
