
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstring>
#include <span>
#include <utility>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/items/parse_context.hpp>
#include <dplx/dp/streams/input_buffer.hpp>

#include "test_utils.hpp"

namespace dp_tests
{

// the class is final and none of its base classes have public destructors
// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class simple_test_input_stream final : public dp::input_buffer
{
    std::span<std::byte const> mReadBuffer;
    bool mInitiallyEmpty;

public:
    ~simple_test_input_stream() = default;

    simple_test_input_stream(std::span<std::byte const> const readBuffer,
                             bool const initiallyEmpty = false)
        : input_buffer(nullptr, 0U, readBuffer.size())
        , mReadBuffer(readBuffer)
        , mInitiallyEmpty(initiallyEmpty)
    {
        if (!initiallyEmpty)
        {
            reset(mReadBuffer.data(), mReadBuffer.size(), mReadBuffer.size());
        }
    }

    [[nodiscard]] auto discarded() const noexcept -> std::size_t
    {
        return mInitiallyEmpty ? 0U : mReadBuffer.size() - size();
    }

private:
    auto do_require_input(size_type requiredSize) noexcept
            -> dp::result<void> override
    {
        if (!std::exchange(mInitiallyEmpty, false)
            || requiredSize > mReadBuffer.size())
        {
            return dp::errc::end_of_stream;
        }
        reset(mReadBuffer.data(), mReadBuffer.size(), mReadBuffer.size());
        return dp::oc::success();
    }
    auto do_discard_input(size_type amount) noexcept
            -> dp::result<void> override
    {
        if (!std::exchange(mInitiallyEmpty, false)
            || amount > mReadBuffer.size())
        {
            return dp::errc::end_of_stream;
        }
        auto const remaining = mReadBuffer.subspan(amount);
        reset(remaining.data(), remaining.size(), remaining.size());
        return dp::oc::success();
    }
    auto do_bulk_read(std::byte *dest, std::size_t size) noexcept
            -> dp::result<void> override
    {
        if (!std::exchange(mInitiallyEmpty, false) || size > mReadBuffer.size())
        {
            return dp::errc::end_of_stream;
        }

        std::memcpy(dest, mReadBuffer.data(), size);

        auto const remaining = mReadBuffer.subspan(size);
        reset(remaining.data(), remaining.size(), remaining.size());
        return dp::oc::success();
    }
};

class simple_test_parse_context final
{
    dp::parse_context ctx;

public:
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    simple_test_input_stream stream;

    explicit simple_test_parse_context(
            std::span<std::byte const> const readBuffer,
            bool const initiallyEmpty = false)
        : ctx{stream}
        , stream(readBuffer, initiallyEmpty)
    {
    }

    auto as_parse_context() noexcept -> dp::parse_context &
    {
        return ctx;
    }
};

} // namespace dp_tests
