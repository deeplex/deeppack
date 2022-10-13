
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/streams/memory_output_stream.hpp"

#include "boost-test.hpp"
#include "test_utils.hpp"

// NOLINTBEGIN(readability-magic-numbers)

namespace dp_tests
{

static_assert(dp::output_stream<dp::memory_buffer>);
static_assert(!dp::lazy_output_stream<dp::memory_buffer>);
static_assert(dp::stream_traits<dp::memory_buffer>::nothrow_output);

BOOST_AUTO_TEST_SUITE(streams)

struct memory_output_stream_dependencies
{
    static constexpr std::size_t testSize = 67;
    std::vector<std::byte> memory{testSize, std::byte{0xfeU}};

    dp::memory_buffer subject{memory.data(), testSize, 0};

    // NOLINTNEXTLINE(modernize-use-equals-default)
    memory_output_stream_dependencies()
    {
    }
};

BOOST_FIXTURE_TEST_SUITE(memory_output_stream,
                         memory_output_stream_dependencies)

using dp::memory_buffer;

BOOST_AUTO_TEST_CASE(spans_whole_buffer)
{
    auto wrx = dp::write(subject, testSize);
    DPLX_REQUIRE_RESULT(wrx);

    auto proxy = wrx.assume_value();
    BOOST_TEST(proxy.size() == testSize);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests

// NOLINTEND(readability-magic-numbers)
