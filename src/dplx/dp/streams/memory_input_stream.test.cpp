
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/streams/memory_input_stream.hpp"

#include <array>

#include <catch2/catch_test_macros.hpp>

#include "dplx/dp/api.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

static_assert(std::derived_from<dp::memory_input_stream, dp::input_buffer>);
static_assert(dp::input_stream<dp::memory_input_stream>);
static_assert(dp::input_stream<dp::memory_input_stream &>);
static_assert(std::semiregular<dp::memory_input_stream>);

static_assert(dp::input_stream<std::span<std::byte> &&>);
static_assert(dp::input_stream<std::span<std::byte const> &&>);
static_assert(!dp::input_stream<std::span<std::byte> &>);
static_assert(!dp::input_stream<std::span<std::byte const> &>);

TEST_CASE("memory_input_stream should be default constructable")
{
    dp::memory_input_stream subject;

    REQUIRE(subject.empty());

    SECTION("should be copyable")
    {
        dp::memory_input_stream copy(subject);
        CHECK(copy.data() == subject.data());
        CHECK(copy.size() == subject.size());
        CHECK(copy.input_size() == subject.input_size());
    }
    SECTION("should not be able to require more input")
    {
        CHECK(subject.require_input(1U).error() == dp::errc::end_of_stream);
    }
    SECTION("should not be able to read any data")
    {
        std::array<std::byte, 1U> storage{};
        CHECK(subject.bulk_read(storage).error() == dp::errc::end_of_stream);
    }
    SECTION("should not be able to discard any input")
    {
        CHECK(subject.discard_input(1U).error() == dp::errc::end_of_stream);
    }
    SECTION("should pass through get_input_buffer()")
    {
        CHECK(&subject == &dp::get_input_buffer(subject));
    }
}

TEST_CASE("memory_input_stream rvalues can pass through get_input_buffer()")
{
    auto &&subject = dp::get_input_buffer(dp::memory_input_stream{});
    CHECK(subject.empty());
}

TEST_CASE("memory_input_stream should be constructable with an empty span")
{
    std::span<std::byte const> bytes;
    dp::memory_input_stream subject(bytes);

    REQUIRE(subject.empty());

    SECTION("should not be able to require more input")
    {
        CHECK(subject.require_input(1U).error() == dp::errc::end_of_stream);
    }
    SECTION("should not be able to read any data")
    {
        std::array<std::byte, 1U> storage{};
        CHECK(subject.bulk_read(storage).error() == dp::errc::end_of_stream);
    }
    SECTION("should not be able to discard any input")
    {
        CHECK(subject.discard_input(1U).error() == dp::errc::end_of_stream);
    }
}

TEST_CASE("memory_input_stream should be constructable given memory")
{
    constexpr std::size_t storageSize = 16U;
    std::array<std::byte, storageSize> storage{};
    dp::memory_input_stream subject(storage);

    SECTION("should forward the underlying storage")
    {
        CHECK(subject.data() == storage.data());
        CHECK(subject.size() == storageSize);
        CHECK(subject.input_size() == storageSize);
    }
    SECTION("should be copyable")
    {
        dp::memory_input_stream copy(subject);
        CHECK(copy.data() == subject.data());
        CHECK(copy.size() == subject.size());
        CHECK(copy.input_size() == subject.input_size());
    }
    SECTION("should not be able to require more input")
    {
        CHECK(subject.require_input(storageSize + 1U).error()
              == dp::errc::end_of_stream);
    }
    SECTION("should not be bulk_readable beyond the given storage")
    {
        std::array<std::byte, storageSize + 1U> toBeRead{};

        CHECK(subject.bulk_read(toBeRead).error() == dp::errc::end_of_stream);
    }
    SECTION("should not be able to discard any more input")
    {
        CHECK(subject.discard_input(storageSize + 1U).error()
              == dp::errc::end_of_stream);
    }
}

TEST_CASE("rvalue byte span can be converted to an input_stream")
{
    std::array<std::byte, 1> storage{};
    auto &&buffer = dp::get_input_buffer(std::span<std::byte>(storage));
    static_assert(std::is_same_v<std::remove_reference_t<decltype(buffer)>,
                                 dp::memory_input_stream>);
    CHECK(buffer.data() == storage.data());
    CHECK(buffer.size() == 1U);
    CHECK(buffer.input_size() == 1U);
}
TEST_CASE("rvalue const byte span can be converted to an input_stream")
{
    std::array<std::byte, 1> storage{};
    auto &&buffer = dp::get_input_buffer(std::span<std::byte const>(storage));
    static_assert(std::is_same_v<std::remove_reference_t<decltype(buffer)>,
                                 dp::memory_input_stream>);
    CHECK(buffer.data() == storage.data());
    CHECK(buffer.size() == 1U);
    CHECK(buffer.input_size() == 1U);
}

} // namespace dp_tests
