
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/streams/memory_input_stream.hpp>

#include <cstddef>

#include <array>
#include <vector>

#include "boost-test.hpp"
#include "test_utils.hpp"

static_assert(dplx::dp::input_stream<dplx::dp::byte_buffer_view>);
static_assert(!dplx::dp::lazy_input_stream<dplx::dp::byte_buffer_view>);

static_assert(dplx::dp::input_stream<dplx::dp::const_byte_buffer_view>);
static_assert(!dplx::dp::lazy_input_stream<dplx::dp::const_byte_buffer_view>);

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(streams)

struct memory_input_stream_dependencies
{
    static constexpr std::size_t testSize = 67;
    std::vector<std::byte> memory{testSize};

    dplx::dp::const_byte_buffer_view subject{std::span<std::byte>(memory)};

    memory_input_stream_dependencies()
    {
        for (auto i = 0u; i < testSize; ++i)
        {
            memory[i] = static_cast<std::byte>(i);
        }
    }
};

BOOST_FIXTURE_TEST_SUITE(memory_input_stream, memory_input_stream_dependencies)

BOOST_AUTO_TEST_CASE(spans_whole_buffer)
{
    auto availableRx = dplx::dp::available_input_size(subject);
    DPLX_REQUIRE_RESULT(availableRx);

    auto const available = availableRx.assume_value();
    BOOST_TEST(available == testSize);
}

BOOST_AUTO_TEST_CASE(correctly_handles_small_reads)
{
    auto readRx = dplx::dp::read(subject, 29);
    DPLX_REQUIRE_RESULT(readRx);
    auto proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 0);
    BOOST_TEST(std::ranges::size(proxy) == 29u);

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value()
                       == (testSize - 29));

    readRx = dplx::dp::read(subject, 31);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 29);
    BOOST_TEST(std::ranges::size(proxy) == 31u);

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value()
                       == (testSize - 29 - 31));

    readRx = dplx::dp::read(subject, 7);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 60);
    BOOST_TEST(std::ranges::size(proxy) == 7u);

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value() == 0u);

    readRx = dplx::dp::read(subject, 1);
    BOOST_TEST_REQUIRE(readRx.has_error());
    BOOST_TEST(readRx.assume_error() == dplx::dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE(correctly_handles_large_reads)
{
    std::vector<std::byte> readBuffer1(23);
    DPLX_REQUIRE_RESULT(
            dplx::dp::read(subject, readBuffer1.data(), readBuffer1.size()));

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value()
                       == (testSize - readBuffer1.size()));
    BOOST_TEST(std::span(readBuffer1) == std::span(memory).subspan(0, 23),
               boost::test_tools::per_element());

    std::vector<std::byte> readBuffer2(37);
    DPLX_REQUIRE_RESULT(
            dplx::dp::read(subject, readBuffer2.data(), readBuffer2.size()));

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value()
                       == (testSize - readBuffer1.size() - readBuffer2.size()));
    BOOST_TEST(std::span(readBuffer2) == std::span(memory).subspan(23, 37),
               boost::test_tools::per_element());

    std::vector<std::byte> readBuffer3(7);
    DPLX_REQUIRE_RESULT(
            dplx::dp::read(subject, readBuffer3.data(), readBuffer3.size()));

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value() == 0u);
    BOOST_TEST(std::span(readBuffer3) == std::span(memory).subspan(60, 7),
               boost::test_tools::per_element());

    std::vector<std::byte> readBuffer4(1);
    auto readRx
            = dplx::dp::read(subject, readBuffer4.data(), readBuffer4.size());
    BOOST_TEST_REQUIRE(readRx.has_error());
    BOOST_TEST(readRx.assume_error() == dplx::dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE(correctly_handles_mixed_reads)
{
    auto readRx = dplx::dp::read(subject, 17);
    DPLX_REQUIRE_RESULT(readRx);
    auto proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 0);
    BOOST_TEST(std::ranges::size(proxy) == 17u);

    std::vector<std::byte> readBuffer2(47);
    DPLX_REQUIRE_RESULT(
            dplx::dp::read(subject, readBuffer2.data(), readBuffer2.size()));

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value()
                       == (testSize - 17 - readBuffer2.size()));
    BOOST_TEST(std::span(readBuffer2) == std::span(memory).subspan(17, 47),
               boost::test_tools::per_element());

    readRx = dplx::dp::read(subject, 3);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 64);
    BOOST_TEST(std::ranges::size(proxy) == 3u);

    readRx = dplx::dp::read(subject, 1);
    BOOST_TEST_REQUIRE(readRx.has_error());
    BOOST_TEST(readRx.assume_error() == dplx::dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE(correctly_handles_partial_reads)
{
    auto readRx = dplx::dp::read(subject, 31);
    DPLX_REQUIRE_RESULT(readRx);
    auto proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 0);
    BOOST_TEST(std::ranges::size(proxy) == 31u);

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value()
                       == (testSize - 31));

    DPLX_REQUIRE_RESULT(dplx::dp::consume(subject, proxy, 29));

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value()
                       == (testSize - 29));

    readRx = dplx::dp::read(subject, 5);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 29);
    BOOST_TEST(std::ranges::size(proxy) == 5u);

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value()
                       == (testSize - 29 - 5));

    DPLX_REQUIRE_RESULT(dplx::dp::consume(subject, proxy, 3));

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value()
                       == (testSize - 29 - 3));

    readRx = dplx::dp::read(subject, 35);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 32);
    BOOST_TEST(std::ranges::size(proxy) == 35u);

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value() == 0u);

    readRx = dplx::dp::read(subject, 1);
    BOOST_TEST_REQUIRE(readRx.has_error());
    BOOST_TEST(readRx.assume_error() == dplx::dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
