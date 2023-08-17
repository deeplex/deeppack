
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/streams/dynamic_memory_output_stream.hpp"

#include <array>

#include <catch2/catch_test_macros.hpp>

#include <dplx/dp/detail/workaround.hpp>

#include "blob_matcher.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

TEST_CASE("dynamic_memory_output_stream should be default constructible")
{
    dp::dynamic_memory_output_stream<std::allocator<std::byte>> subject;
    CHECK(subject.empty());
    CHECK(subject.written_size() == 0U);
    CHECK(subject.written().empty());

    SECTION("and copyable")
    {
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        auto const copy = subject;
        CHECK(copy.empty());
        CHECK(copy.written_size() == 0U);
        CHECK(copy.written().empty());
    }
    SECTION("and growable")
    {
        REQUIRE(subject.ensure_size(1U));
        CHECK(!subject.empty());
        CHECK(subject.data() != nullptr);
    }
    SECTION("and bulk_writable")
    {
        constexpr std::byte invalidItem{0xfe};
        std::array<std::byte, 1U> storage{invalidItem};
        CHECK(subject.bulk_write(storage.data(), storage.size()));
        CHECK(subject.written_size() == storage.size());
        CHECK_BLOB_EQ(subject.written(), storage);
    }
}

TEST_CASE("dynamic_memory_output_stream can be constructed from an existing "
          "vector")
{
    constexpr std::size_t preallocationSize = 127U;
    std::vector<std::byte> existing(preallocationSize, std::byte{});
    auto const *const memPtr = existing.data();
    dp::dynamic_memory_output_stream subject(std::move(existing));

    CHECK(subject.size() == preallocationSize);
    CHECK(subject.data() == memPtr);
    CHECK(subject.written_size() == 0U);
    CHECK(existing.empty()); // NOLINT(bugprone-use-after-move)

    existing = std::move(subject).written();

    CHECK(existing.empty());
    CHECK(existing.capacity() == preallocationSize);
    CHECK(existing.data() == memPtr);
    CHECK(subject.empty()); // NOLINT(bugprone-use-after-move)
}

} // namespace dp_tests
