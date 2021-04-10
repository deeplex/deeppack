
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

namespace dp_tests
{

static_assert(dp::input_stream<dp::byte_buffer_view>);
static_assert(!dp::lazy_input_stream<dp::byte_buffer_view>);
static_assert(dp::stream_traits<dp::byte_buffer_view>::nothrow_input);

static_assert(dp::input_stream<dp::const_byte_buffer_view>);
static_assert(!dp::lazy_input_stream<dp::const_byte_buffer_view>);
static_assert(dp::stream_traits<dp::const_byte_buffer_view>::nothrow_input);

BOOST_AUTO_TEST_SUITE(streams)

struct memory_input_stream_dependencies
{
    static constexpr std::size_t testSize = 67;
    std::vector<std::byte> memory{testSize};

    dp::const_byte_buffer_view subject{std::span<std::byte>(memory)};

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
    auto availableRx = dp::available_input_size(subject);
    DPLX_REQUIRE_RESULT(availableRx);

    auto const available = availableRx.assume_value();
    BOOST_TEST(available == testSize);
}

BOOST_AUTO_TEST_CASE(correctly_handles_small_reads)
{
    auto readRx = dp::read(subject, 29);
    DPLX_REQUIRE_RESULT(readRx);
    auto proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 0);
    BOOST_TEST(std::ranges::size(proxy) == 29u);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29));

    readRx = dp::read(subject, 31);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 29);
    BOOST_TEST(std::ranges::size(proxy) == 31u);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31));

    readRx = dp::read(subject, 7);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 60);
    BOOST_TEST(std::ranges::size(proxy) == 7u);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value() == 0u);

    readRx = dp::read(subject, 1);
    BOOST_TEST_REQUIRE(readRx.has_error());
    BOOST_TEST(readRx.assume_error() == dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE(correctly_handles_large_reads)
{
    std::vector<std::byte> readBuffer1(23);
    DPLX_REQUIRE_RESULT(
            dp::read(subject, readBuffer1.data(), readBuffer1.size()));

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - readBuffer1.size()));
    BOOST_TEST(std::span(readBuffer1) == std::span(memory).subspan(0, 23),
               boost::test_tools::per_element());

    std::vector<std::byte> readBuffer2(37);
    DPLX_REQUIRE_RESULT(
            dp::read(subject, readBuffer2.data(), readBuffer2.size()));

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - readBuffer1.size() - readBuffer2.size()));
    BOOST_TEST(std::span(readBuffer2) == std::span(memory).subspan(23, 37),
               boost::test_tools::per_element());

    std::vector<std::byte> readBuffer3(7);
    DPLX_REQUIRE_RESULT(
            dp::read(subject, readBuffer3.data(), readBuffer3.size()));

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value() == 0u);
    BOOST_TEST(std::span(readBuffer3) == std::span(memory).subspan(60, 7),
               boost::test_tools::per_element());

    std::vector<std::byte> readBuffer4(1);
    auto readRx = dp::read(subject, readBuffer4.data(), readBuffer4.size());
    BOOST_TEST_REQUIRE(readRx.has_error());
    BOOST_TEST(readRx.assume_error() == dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE(correctly_handles_mixed_reads)
{
    auto readRx = dp::read(subject, 17);
    DPLX_REQUIRE_RESULT(readRx);
    auto proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 0);
    BOOST_TEST(std::ranges::size(proxy) == 17u);

    std::vector<std::byte> readBuffer2(47);
    DPLX_REQUIRE_RESULT(
            dp::read(subject, readBuffer2.data(), readBuffer2.size()));

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 17 - readBuffer2.size()));
    BOOST_TEST(std::span(readBuffer2) == std::span(memory).subspan(17, 47),
               boost::test_tools::per_element());

    readRx = dp::read(subject, 3);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 64);
    BOOST_TEST(std::ranges::size(proxy) == 3u);

    readRx = dp::read(subject, 1);
    BOOST_TEST_REQUIRE(readRx.has_error());
    BOOST_TEST(readRx.assume_error() == dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE(correctly_handles_partial_reads)
{
    auto readRx = dp::read(subject, 31);
    DPLX_REQUIRE_RESULT(readRx);
    auto proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 0);
    BOOST_TEST(std::ranges::size(proxy) == 31u);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 31));

    DPLX_REQUIRE_RESULT(dp::consume(subject, proxy, 29));

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29));

    readRx = dp::read(subject, 5);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 29);
    BOOST_TEST(std::ranges::size(proxy) == 5u);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 5));

    DPLX_REQUIRE_RESULT(dp::consume(subject, proxy, 3));

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 3));

    readRx = dp::read(subject, 35);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 32);
    BOOST_TEST(std::ranges::size(proxy) == 35u);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value() == 0u);

    readRx = dp::read(subject, 1);
    BOOST_TEST_REQUIRE(readRx.has_error());
    BOOST_TEST(readRx.assume_error() == dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE(properly_skips_bytes_at_beginning_1)
{
    auto skipRx = dp::skip_bytes(subject, 17);
    DPLX_REQUIRE_RESULT(skipRx);

    auto sreadRx = dp::read(subject, 50);
    DPLX_REQUIRE_RESULT(sreadRx);
    auto proxy = sreadRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == memory.data() + 17);
    BOOST_TEST(std::ranges::size(proxy) == 50u);
}

BOOST_AUTO_TEST_CASE(skips_correctly_from_sbo_1)
{
    auto sreadRx = dp::read(subject, 17);
    DPLX_REQUIRE_RESULT(sreadRx);
    auto proxy = sreadRx.assume_value();
    BOOST_TEST_REQUIRE(std::ranges::size(proxy) == 17u);

    sreadRx = dp::read(subject, 40);
    DPLX_REQUIRE_RESULT(sreadRx);
    proxy = sreadRx.assume_value();
    BOOST_TEST_REQUIRE(std::ranges::size(proxy) == 40u);

    auto consumeRx = dp::consume(subject, proxy, 20u);
    DPLX_REQUIRE_RESULT(consumeRx);

    auto skipRx = dp::skip_bytes(subject, 13u);
    DPLX_REQUIRE_RESULT(skipRx);

    sreadRx = dp::read(subject, 17u);
    DPLX_REQUIRE_RESULT(sreadRx);
    proxy = sreadRx.assume_value();
    BOOST_TEST_REQUIRE(std::ranges::size(proxy) == 17u);
    BOOST_TEST_REQUIRE(std::ranges::data(proxy) == memory.data() + 50);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
