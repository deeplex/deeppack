
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/streams/memory_output_stream.hpp>

#include "boost-test.hpp"
#include "test_utils.hpp"

static_assert(dplx::dp::output_stream<dplx::dp::byte_buffer_view>);
static_assert(!dplx::dp::lazy_output_stream<dplx::dp::byte_buffer_view>);

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(streams)

struct memory_output_stream_dependencies
{
    static constexpr std::size_t testSize = 67;
    std::vector<std::byte> memory{testSize, std::byte{0xfeu}};

    dplx::dp::byte_buffer_view subject{memory.data(), testSize, 0};

    memory_output_stream_dependencies()
    {
    }
};

BOOST_FIXTURE_TEST_SUITE(memory_output_stream,
                         memory_output_stream_dependencies)

using dplx::dp::byte_buffer_view;

BOOST_AUTO_TEST_CASE(spans_whole_buffer)
{
    auto wrx = dplx::dp::write(subject, testSize);
    DPLX_REQUIRE_RESULT(wrx);

    auto proxy = wrx.assume_value();
    BOOST_TEST(proxy.size() == testSize);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
