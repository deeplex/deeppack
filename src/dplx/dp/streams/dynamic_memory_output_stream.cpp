
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/streams/dynamic_memory_output_stream.hpp"

namespace dplx::dp
{

// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
template class dynamic_memory_output_stream<std::allocator<std::byte>>;

} // namespace dplx::dp
