
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/streams/output_buffer.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <ranges>

#include <catch2/catch_test_macros.hpp>

#include "test_utils.hpp"

namespace dp_tests
{

// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class test_output_buffer final : public dp::output_buffer
{
public:
    int grow_calls{0};
    size_type last_grow_param{};
    int bulk_write_calls{0};
    std::byte const *last_bulk_write_call_param0{};
    std::size_t last_bulk_write_call_param1{};

    test_output_buffer() = default;
    explicit test_output_buffer(std::byte *const buffer,
                                size_type const size) noexcept
        : output_buffer(buffer, size)
    {
    }

    using output_buffer::reset;

    auto do_grow(size_type const requestedSize) noexcept
            -> result<void> override
    {
        grow_calls += 1;
        last_grow_param = requestedSize;
        return dp::errc::end_of_stream;
    }

    auto do_bulk_write(std::byte const *const bytes,
                       std::size_t const bytesSize) noexcept
            -> result<void> override
    {
        bulk_write_calls += 1;
        last_bulk_write_call_param0 = bytes;
        last_bulk_write_call_param1 = bytesSize;
        return dp::errc::end_of_stream;
    }
};

static_assert(std::ranges::output_range<test_output_buffer, std::byte>,
              "test_output_buffer is on output_range");
static_assert(std::ranges::contiguous_range<test_output_buffer>,
              "test_output_buffer is a contiguous range");
static_assert(std::ranges::sized_range<test_output_buffer>,
              "test_output_buffer is a sized range");

TEST_CASE("output_buffer can be default constructed")
{
    test_output_buffer subject;

    SECTION("it is an empty range")
    {
        CHECK(subject.begin() == subject.end());
        // NOLINTNEXTLINE(readability-container-size-empty)
        CHECK(subject.size() == 0U);
        CHECK(subject.empty());
    }

    SECTION("reset updates the range buffer")
    {
        constexpr std::size_t testMemorySize = 64U;
        std::array<std::byte, testMemorySize> storage{};
        subject.reset(storage.data(), storage.size());

        CHECK(subject.data() == storage.data());
        CHECK(subject.size() == storage.size());
        CHECK_FALSE(subject.empty());
        CHECK(std::ranges::distance(subject.begin(), subject.end())
              == std::ranges::ssize(storage));
    }

    SECTION("calling ensure_size calls do_grow in turn")
    {
        CHECK(subject.grow_calls == 0);

        auto &&reserveRx = subject.ensure_size(1);
        CHECK(reserveRx.has_error());
        CHECK(reserveRx.error() == dp::errc::end_of_stream);

        CHECK(subject.grow_calls == 1);
        CHECK(subject.last_grow_param == 1U);
    }

    SECTION("calling bulk_write calls do_bulk_write in turn")
    {
        constexpr std::size_t testMemorySize = 64U;
        std::array<std::byte, testMemorySize> memory{};

        CHECK(subject.bulk_write(memory.data(), memory.size()).error()
              == dp::errc::end_of_stream);
        CHECK(subject.grow_calls == 0);
        CHECK(subject.bulk_write_calls == 1);
        CHECK(subject.last_bulk_write_call_param0 == memory.data());
        CHECK(subject.last_bulk_write_call_param1 == memory.size());
        CHECK(subject.empty());
    }

    SECTION("calling bulk_write with size=0 does nothing")
    {
        CHECK(subject.bulk_write(nullptr, 0U));
        CHECK(subject.grow_calls == 0);
        CHECK(subject.bulk_write_calls == 0);
        CHECK(subject.empty());
    }
}

TEST_CASE("output_buffer can be constructed with an initial buffer")
{
    constexpr std::size_t testMemorySize = 64U;
    std::array<std::byte, testMemorySize> initialStorage{};
    test_output_buffer subject(initialStorage.data(), initialStorage.size());

    SECTION("it points to initial storage")
    {
        CHECK(subject.data() == initialStorage.data());
        CHECK(subject.size() == initialStorage.size());
        CHECK_FALSE(subject.empty());
        CHECK(std::ranges::distance(subject.begin(), subject.end())
              == std::ranges::ssize(initialStorage));
    }

    SECTION("reset without params clears the buffer")
    {
        subject.reset();
        CHECK(subject.begin() == subject.end());
        // NOLINTNEXTLINE(readability-container-size-empty)
        CHECK(subject.size() == 0U);
        CHECK(subject.empty());
    }

    SECTION("ensure_size calls grow only if necessary")
    {
        CHECK(subject.grow_calls == 0);

        CHECK(subject.ensure_size(initialStorage.size()));
        CHECK(subject.size() == initialStorage.size());

        CHECK(subject.grow_calls == 0);
    }

    SECTION("commit_written shrinks the range by advancing begin")
    {
        subject.commit_written(1U);

        CHECK(subject.size() == initialStorage.size() - 1U);
        CHECK(subject.data() > initialStorage.data());
    }

    SECTION("calling bulk_write calls do_bulk_write in turn")
    {
        constexpr std::byte fillValue{0xcc};
        std::array<std::byte, testMemorySize * 2> memory{};
        std::ranges::fill(memory, fillValue);

        CHECK(subject.bulk_write(memory.data(), memory.size()).error()
              == dp::errc::end_of_stream);
        CHECK(subject.grow_calls == 0);
        CHECK(subject.bulk_write_calls == 1);
        CHECK(subject.last_bulk_write_call_param0
              == memory.data() + initialStorage.size());
        CHECK(subject.last_bulk_write_call_param1
              == memory.size() - initialStorage.size());
        CHECK(subject.empty());
    }

    SECTION("calling bulk_write with size=0 does nothing")
    {
        CHECK(subject.bulk_write(nullptr, 0U));
        CHECK(subject.grow_calls == 0);
        CHECK(subject.bulk_write_calls == 0);
        CHECK(subject.size() == initialStorage.size());
    }
}

} // namespace dp_tests
