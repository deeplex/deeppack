
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/streams/memory_output_stream.hpp"

#include <array>

#include <catch2/catch_test_macros.hpp>

#include "test_utils.hpp"

namespace dp_tests
{

static_assert(std::is_base_of_v<dp::output_buffer, dp::memory_output_stream>);

TEST_CASE("memory_output_stream should be constructable with an empty span")
{
    std::array<std::byte, 1> storage{};
    std::span<std::byte> bytes;
    dp::memory_output_stream subject(bytes);

    REQUIRE(subject.empty());

    SECTION("should not be growable")
    {
        CHECK(subject.ensure_size(1U).error() == dp::errc::end_of_stream);
    }

    SECTION("should not be bulk_writable")
    {
        CHECK(subject.bulk_write(storage.data(), storage.size()).error()
              == dp::errc::end_of_stream);
    }
}

TEST_CASE("memory_output_stream should be constructable given memory")
{
    constexpr std::size_t storageSize = 16U;
    std::array<std::byte, storageSize> storage{};
    dp::memory_output_stream subject(storage);

    SECTION("should forward the underlying storage")
    {
        CHECK(subject.size() == storageSize);
        CHECK(subject.data() == storage.data());
    }

    SECTION("should not be growable")
    {
        CHECK(subject.ensure_size(storageSize + 1U).error()
              == dp::errc::end_of_stream);
    }

    SECTION("should not be bulk_writable beyond the given storage")
    {
        std::array<std::byte, storageSize + 1U> toBeWritten{};

        CHECK(subject.bulk_write(toBeWritten.data(), toBeWritten.size()).error()
              == dp::errc::end_of_stream);
    }
}

} // namespace dp_tests
