
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/core.hpp"

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <dplx/cncr/misc.hpp>

#include "dplx/dp/encoder/api.hpp"
#include "dplx/dp/streams/memory_output_stream2.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

TEMPLATE_TEST_CASE("integers are encodable",
                   "",
                   unsigned char,
                   signed char,
                   unsigned short,
                   short,
                   unsigned,
                   int,
                   unsigned long,
                   long,
                   unsigned long long,
                   long long)
{
    static_assert(dp::ng::encodable<TestType>);
    auto const encoded = cncr::make_byte_array<2U>({0x18, 0x7e});

    std::vector<std::byte> encodingBuffer(encoded.size());
    dp::memory_output_stream outputStream(encodingBuffer);

    REQUIRE(dp::encode(outputStream, static_cast<TestType>(encoded[1])));

    CHECK(std::ranges::equal(outputStream.written(), encoded));
}

} // namespace dp_tests
