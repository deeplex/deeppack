
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/cpos/stream.hpp"

#include <catch2/catch_test_macros.hpp>

#include <dplx/dp/api.hpp>
#include <dplx/dp/codecs/core.hpp>
#include <dplx/dp/streams/input_buffer.hpp>
#include <dplx/dp/streams/output_buffer.hpp>

#include "test_utils.hpp"

namespace dp_tests
{

namespace
{

class movable_stream final
    : public dp::input_buffer
    , public dp::output_buffer
{
public:
    ~movable_stream() noexcept = default;
    movable_stream() noexcept = default;

    movable_stream(movable_stream const &) noexcept = delete;
    auto operator=(movable_stream const &) noexcept
            -> movable_stream & = delete;

    movable_stream(movable_stream &&) noexcept
        : input_buffer()
        , output_buffer()
    {
    }
    auto operator=(movable_stream &&) noexcept -> movable_stream &
    {
        return *this;
    }

private:
    auto do_require_input(input_buffer::size_type) noexcept
            -> dp::result<void> override
    {
        return dp::errc::bad;
    }
    auto do_discard_input(input_buffer::size_type) noexcept
            -> dp::result<void> override
    {
        return dp::errc::bad;
    }
    auto do_bulk_read(std::byte *, std::size_t) noexcept
            -> dp::result<void> override
    {
        return dp::errc::bad;
    }

    auto do_grow(output_buffer::size_type) noexcept -> dp::result<void> override
    {
        return dp::errc::bad;
    }
    auto do_bulk_write(std::byte const *, std::size_t) noexcept
            -> dp::result<void> override
    {
        return dp::errc::bad;
    }
};

} // namespace

static_assert(dp::input_stream<movable_stream>);
static_assert(dp::input_stream<movable_stream &>);
static_assert(!dp::output_stream<movable_stream>);
static_assert(dp::output_stream<movable_stream &>);

TEST_CASE("movable_stream can be passed as lvalue to encode")
{
    movable_stream stream{};
    CHECK(dp::encode(stream, dp::null_value).error() == dp::errc::bad);
}

TEST_CASE("movable_stream can be passed as lvalue to decode")
{
    movable_stream stream{};
    CHECK(dp::decode(dp::as_value<int>, stream).error()
          == dp::errc::end_of_stream);
}
TEST_CASE("movable_stream can be passed as rvalue to decode")
{
    CHECK(dp::decode(dp::as_value<int>, movable_stream{}).error()
          == dp::errc::end_of_stream);
}

} // namespace dp_tests
