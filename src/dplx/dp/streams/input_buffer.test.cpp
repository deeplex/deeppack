
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/streams/input_buffer.hpp"

#include <algorithm>
#include <array>
#include <ranges>

#include <catch2/catch_test_macros.hpp>

#include "test_utils.hpp"

namespace dp_tests
{

// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class test_input_buffer final : public dp::input_buffer
{
public:
    int require_input_calls{0};
    size_type last_require_input_param{};
    int discard_input_calls{0};
    size_type last_discard_input_param{};
    int bulk_read_calls{0};
    std::byte *last_bulk_read_param0{};
    std::size_t last_bulk_read_param1{};

    test_input_buffer() = default;
    explicit test_input_buffer(pointer const writeBuffer,
                               size_type const size) noexcept
        : input_buffer(writeBuffer, size, size)
    {
    }

    using input_buffer::reset;

    auto do_require_input(size_type const requiredSize) noexcept
            -> dp::result<void> override
    {
        require_input_calls += 1;
        last_require_input_param = requiredSize;
        return dp::errc::end_of_stream;
    }
    auto do_discard_input(size_type const amount) noexcept
            -> dp::result<void> override
    {
        discard_input_calls += 1;
        last_discard_input_param = amount;
        return dp::errc::end_of_stream;
    }
    auto do_bulk_read(std::byte *const dest, std::size_t const size) noexcept
            -> dp::result<void> override
    {
        bulk_read_calls += 1;
        last_bulk_read_param0 = dest;
        last_bulk_read_param1 = size;
        return dp::errc::end_of_stream;
    }
};

static_assert(std::ranges::input_range<test_input_buffer>,
              "test_input_buffer is on input_range");
static_assert(std::ranges::contiguous_range<test_input_buffer>,
              "test_input_buffer is a contiguous range");
static_assert(std::ranges::sized_range<test_input_buffer>,
              "test_input_buffer is a sized range");

TEST_CASE("input_buffer can be default constructed")
{
    test_input_buffer subject;

    SECTION("is an empty range")
    {
        CHECK(subject.begin() == subject.end());
        CHECK(subject.size() == 0U); // NOLINT(readability-container-size-empty)
        CHECK(subject.empty());
    }

    SECTION("reset updates the range buffer")
    {
        constexpr std::size_t testMemorySize = 64U;
        std::array<std::byte, testMemorySize> storage{};
        subject.reset(storage.data(), storage.size(), storage.size());

        CHECK(subject.data() == storage.data());
        CHECK(subject.size() == storage.size());
        CHECK_FALSE(subject.empty());
        CHECK(std::ranges::distance(subject.begin(), subject.end())
              == std::ranges::ssize(storage));
    }

    SECTION("calling require_input calls do_require_input in turn")
    {
        CHECK(subject.require_input_calls == 0);

        CHECK(subject.require_input(1U).error() == dp::errc::end_of_stream);

        CHECK(subject.require_input_calls == 1);
        CHECK(subject.last_require_input_param == 1U);
    }

    SECTION("calling discard_input calls do_discard_input in turn")
    {
        CHECK(subject.discard_input_calls == 0);

        CHECK(subject.discard_input(1U).error() == dp::errc::end_of_stream);

        CHECK(subject.discard_input_calls == 1);
        CHECK(subject.last_discard_input_param == 1U);
    }

    SECTION("calling bulk_read calls do_bulk_read in turn")
    {
        CHECK(subject.bulk_read_calls == 0);

        std::array<std::byte, 1U> writeArea{};
        CHECK(subject.bulk_read(writeArea.data(), writeArea.size()).error()
              == dp::errc::end_of_stream);

        CHECK(subject.bulk_read_calls == 1);
        CHECK(subject.last_bulk_read_param0 == writeArea.data());
        CHECK(subject.last_bulk_read_param1 == writeArea.size());
    }

    SECTION("calling bulk_read with size=0 does nothing")
    {
        CHECK(subject.bulk_read(nullptr, 0U));
        CHECK(subject.require_input_calls == 0);
        CHECK(subject.bulk_read_calls == 0);
        CHECK(subject.empty());
    }
}

TEST_CASE("input_buffer can be constructed with an initial buffer")
{
    constexpr std::size_t testMemorySize = 64U;
    std::array<std::byte, testMemorySize> initialStorage{};
    test_input_buffer subject(initialStorage.data(), initialStorage.size());

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
        CHECK(subject.size() == 0U); // NOLINT(readability-container-size-empty)
        CHECK(subject.empty());
    }

    SECTION("require_input calls do_require_input only if necessary")
    {
        CHECK(subject.require_input_calls == 0);

        CHECK(subject.require_input(initialStorage.size()));
        CHECK(subject.size() == initialStorage.size());

        CHECK(subject.require_input_calls == 0);
    }

    SECTION("discard_buffered shrinks the range by advancing begin")
    {
        subject.discard_buffered(1U);

        CHECK(subject.size() == initialStorage.size() - 1U);
        CHECK(subject.data() > initialStorage.data());
    }
    SECTION("discard_input shrinks the buffer range by advancing begin")
    {
        CHECK(subject.discard_input(1U));

        CHECK(subject.discard_input_calls == 0);
        CHECK(subject.size() == initialStorage.size() - 1U);
        CHECK(subject.data() > initialStorage.data());
    }
    SECTION("discard_input discards the buffer and calls do_discard_input")
    {
        CHECK(subject.discard_input_calls == 0);

        CHECK(subject.discard_input(initialStorage.size() + 1U).error()
              == dp::errc::end_of_stream);

        CHECK(subject.discard_input_calls == 1);
        CHECK(subject.empty());
        CHECK(subject.last_discard_input_param == 1U);
    }

    SECTION("calling bulk_write calls do_bulk_write in turn")
    {
        std::array<std::byte, testMemorySize * 2> memory{};

        CHECK(subject.bulk_read(memory.data(), memory.size()).error()
              == dp::errc::end_of_stream);

        CHECK(subject.require_input_calls == 0);
        CHECK(subject.bulk_read_calls == 1);
        CHECK(subject.last_bulk_read_param0
              == memory.data() + initialStorage.size());
        CHECK(subject.last_bulk_read_param1
              == memory.size() - initialStorage.size());
        CHECK(subject.empty());
    }
    SECTION("calling bulk_read with size=0 does nothing")
    {
        CHECK(subject.bulk_read(nullptr, 0U));
        CHECK(subject.require_input_calls == 0);
        CHECK(subject.bulk_read_calls == 0);
        CHECK(subject.size() == initialStorage.size());
    }
}

} // namespace dp_tests
