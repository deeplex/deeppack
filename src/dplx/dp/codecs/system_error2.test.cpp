
// Copyright 2023 Henrik Steffen Gaßmann
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/system_error2.hpp"

#include <catch2/catch_test_macros.hpp>

#include <dplx/dp/api.hpp>

#include "blob_matcher.hpp"
#include "item_sample_ct.hpp"
#include "test_input_stream.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

using string_ref = dp::system_error::status_code_domain::string_ref;

TEST_CASE("system_error2::status_code_domain::string_ref should be encodable")
{
    item_sample_ct<std::string_view> const sample{
            "some", 5, {0x64, u8's', u8'o', u8'm', u8'e'}
    };
    string_ref value(sample.value.data(), sample.value.size());

    SECTION("to a stream")
    {
        simple_test_output_stream outputStream(sample.encoded_length);

        REQUIRE(dp::encode(outputStream, value));

        CHECK_BLOB_EQ(outputStream.written(), sample.encoded_bytes());
    }
    SECTION("with a size_of operator")
    {
        CHECK(dp::encoded_size_of(value) == sample.encoded_length);
    }
}

} // namespace dp_tests
