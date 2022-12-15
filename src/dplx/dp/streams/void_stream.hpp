
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/streams/output_buffer.hpp>

namespace dplx::dp
{

// the class is final and none of its base classes have public destructors
// NOLINTBEGIN(cppcoreguidelines-virtual-class-destructor)

/**
 * This output stream just consists of a local buffer which will be recycled
 * over and over again. It is mainly useful for discarding encoder output, e.g.
 * for measuring encoded item size. It is also used as a dummy output buffer for
 * encoded_size_of calls.
 */
class void_stream final : public output_buffer
{
    static constexpr std::size_t void_buffer_size = 4000U;
    std::uint64_t mTotalWritten;
    std::byte mMemory[void_buffer_size];

public:
    // not initializing mMemory sort of is the point…
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    constexpr explicit void_stream() noexcept
        : output_buffer(static_cast<std::byte *>(mMemory), sizeof(mMemory))
        , mTotalWritten(0U)
    {
    }

    [[nodiscard]] auto total_written() const noexcept -> std::uint64_t
    {
        return mTotalWritten + void_buffer_size - size();
    }

private:
    auto do_grow(size_type const requestedSize) noexcept
            -> result<void> override
    {
        if (requestedSize > sizeof(mMemory))
        {
            return errc::end_of_stream;
        }
        mTotalWritten += void_buffer_size - size();
        output_buffer::reset(static_cast<std::byte *>(mMemory),
                             sizeof(mMemory));
        return oc::success();
    }
    auto do_bulk_write(std::byte const *, std::size_t toBeWritten) noexcept
            -> result<void> override
    {
        // we do nothing with the given memory and proactively update the buffer
        // as this method is only called if the "current" buffer is exceeded
        mTotalWritten += void_buffer_size + toBeWritten;
        output_buffer::reset(static_cast<std::byte *>(mMemory),
                             sizeof(mMemory));
        return oc::success();
    }
};

// NOLINTEND(cppcoreguidelines-virtual-class-destructor)

} // namespace dplx::dp
