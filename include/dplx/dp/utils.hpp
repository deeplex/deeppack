
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <boost/endian/conversion.hpp>

namespace dplx::dp::detail
{

template <typename T>
void store(std::byte *dest, T value)
{
    boost::endian::endian_store<T, sizeof(T), boost::endian::order::big>(
        reinterpret_cast<unsigned char *>(dest), value);
}

template <typename T>
auto load(std::byte const *src) noexcept -> T
{
    return boost::endian::endian_load<T, sizeof(T), boost::endian::order::big>(
        reinterpret_cast<unsigned char const *>(src));
}

} // namespace dplx::dp::detail
