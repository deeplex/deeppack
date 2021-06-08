# Written in 2017, 2019 by Henrik Steffen Gaßmann <henrik@gassmann.onl>
#
# To the extent possible under law, the author(s) have dedicated all
# copyright and related and neighboring rights to this software to the
# public domain worldwide. This software is distributed without any warranty.
#
# You should have received a copy of the CC0 Public Domain Dedication
# along with this software. If not, see
#
#     http://creativecommons.org/publicdomain/zero/1.0/
#
########################################################################
cmake_minimum_required(VERSION 3.16)

########################################################################
# configure vcpkg from environment vars if possible

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
endif()

if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()

if(NOT DEFINED VCPKG_FEATURE_FLAGS)
    set(VCPKG_FEATURE_FLAGS "registries" CACHE STRING "")
endif()

option(BUILD_TESTING "Add test targets" OFF)
if (BUILD_TESTING)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

########################################################################
project(deeppack VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/tools/cmake/")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    set(GXX_COMPATIBLE 1)
endif()

if (MSVC OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    set(CL_COMPATIBLE 1)
endif()


########################################################################
# options
option(DP_ASAN "Build with AddressSanitizer" OFF)
option(DPLX_DP_DISABLE_WORKAROUNDS "Disable all workarounds" OFF)
option(DPLX_DP_FLAG_OUTDATED_WORKAROUNDS "Emit compiler errors for workarounds which are active, but haven't been validated for this version" OFF)
option(DPLX_DP_USE_BRANCHING_INTEGER_ENCODER "Use the branching store_var_uint implementation" OFF)
mark_as_advanced(DPLX_DP_USE_BRANCHING_INTEGER_ENCODER)

########################################################################
# dependencies

find_package(fmt CONFIG REQUIRED)

find_package(outcome CONFIG REQUIRED)

if (BUILD_TESTING)

    set(DP_REQUIRE_UNIT_TEST_FRAMEWORK unit_test_framework)
endif()
find_package(Boost 1.71 REQUIRED COMPONENTS
    ${DP_REQUIRE_UNIT_TEST_FRAMEWORK}
)


########################################################################
# warning configuration
# the warning are not added to the library target in order to allow
# consumers to use a different warning configuration
if (CL_COMPATIBLE)
    add_compile_options(
        /W4 # warning level 4
        /WX # treat warnings as errors
        # with the following exceptions
    )
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(
            -Wno-delete-non-abstract-non-virtual-dtor
            -Wmissing-declarations
            -Wredundant-decls
        )
    endif()
elseif (GXX_COMPATIBLE)
    add_compile_options(
        -Wall # easily avoidable code smells
        -Wextra # more code smell warnings
        -pedantic # ISO C++ conformance warnings
        -Werror # treat warnings as errors
        -Wdeprecated
        -Wextra-semi
        -Wcast-align
        -Wconversion
        -Wmissing-declarations
        -Wredundant-decls
        # with the following exceptions
        -Wno-unknown-pragmas # pragma region is not supported :(
    )
endif()


########################################################################
# library
add_library(deeppack)
add_library(Deeplex::deeppack ALIAS deeppack)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated/include/dplx/dp/detail)
target_include_directories(deeppack PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated/include>
    $<INSTALL_INTERFACE:include>
)

configure_file("tools/config.hpp.in" ${CMAKE_CURRENT_BINARY_DIR}/generated/include/dplx/dp/detail/config.hpp @ONLY)

target_sources(deeppack PRIVATE
    "src/disappointment.cpp"
)
set(DP_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/dplx")
target_sources(deeppack PUBLIC
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/config.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated/include/dplx/dp/detail/config.hpp>

    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/concepts.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/fwd.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/layout_descriptor.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/memory_buffer.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/object_def.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/stream.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/tuple_def.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/type_code.hpp>

    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/detail/item_size.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/encoder/api.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/encoder/arg_list.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/encoder/core.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/encoder/narrow_strings.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/encoder/object_utils.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/encoder/tuple_utils.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/item_emitter.hpp>

    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/decoder/api.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/decoder/array_utils.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/decoder/core.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/decoder/object_utils.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/decoder/std_container.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/decoder/std_string.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/decoder/tuple_utils.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/decoder/utils.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/detail/parse_item.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/item_parser.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/skip_item.hpp>

    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/customization.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/customization.std.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/disappointment.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/indefinite_range.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/map_pair.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/tag_invoke.hpp>

    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/streams/chunked_input_stream.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/streams/chunked_output_stream.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/streams/memory_input_stream.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/streams/memory_output_stream.hpp>

    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/detail/bit.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/detail/hash.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/detail/mp_lite.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/detail/mp_for_dots.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/detail/perfect_hash.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/detail/type_utils.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/detail/utils.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/detail/workaround.hpp>
)

target_link_libraries(deeppack PUBLIC
    Boost::boost
    fmt::fmt
    outcome::outcome
)

target_compile_features(deeppack PUBLIC cxx_std_20)
set_target_properties(deeppack PROPERTIES
    CXX_EXTENSIONS OFF
)
if (CL_COMPATIBLE)
    target_compile_options(deeppack PUBLIC
        /utf-8 # all sources are utf-8 encoded
        /permissive- # TPL, disables many MSVC leniencies
        /Zc:__cplusplus # correctly define the __cplusplus macro
    )
endif()

if (WIN32)
    target_compile_definitions(deeppack PUBLIC
        -D_WIN32_WINNT=0x0601 # WinSDK @ Windows 7
        -DUNICODE -D_UNICODE  # force Unicode WinAPIs
    )
endif()

if (DP_ASAN)
    if (CL_COMPATIBLE)
        target_compile_options(deeppack PUBLIC -fsanitize=address)
    elseif (GXX_COMPATIBLE)
        target_compile_options(deeppack PUBLIC -fsanitize=address)
        target_link_options(deeppack PUBLIC -fsanitize=address)
    endif()
endif()

########################################################################
# library test project
if (BUILD_TESTING)
    add_executable(deeppack-tests)
    set_target_properties(deeppack-tests PROPERTIES
        CXX_EXTENSIONS OFF
    )
    target_sources(deeppack-tests PRIVATE
        "tests/deeppack-tests.main.cpp"
        "tests/boost-test.hpp"
        "tests/cpp.hint"

        "tests/test_input_stream.hpp"
        "tests/test_output_stream.hpp"
        "tests/test_utils.hpp"

        "tests/item_emitter.integer.test.cpp"
        "tests/item_emitter.test.cpp"

        "tests/item_parser.array.test.cpp"
        "tests/item_parser.binary.test.cpp"
        "tests/item_parser.expect.test.cpp"
        "tests/item_parser.integer.test.cpp"
        "tests/item_parser.test.cpp"
        
        "tests/decoder.test.cpp"
        "tests/decoder.std_container.test.cpp"
        "tests/decoder.std_string.test.cpp"
        "tests/decoder.object_utils.test.cpp"
        "tests/decoder.tuple_utils.test.cpp"

        "tests/encoder.test.cpp"
        "tests/encoder.test_utils.hpp"
        "tests/encoder.blob.test.cpp"
        "tests/encoder.map.test.cpp"
        "tests/encoder.range.test.cpp"
        "tests/encoder.string.test.cpp"
        "tests/encoder.tuple.test.cpp"
        "tests/encoder.object_utils.test.cpp"
        "tests/encoder.tuple_utils.test.cpp"
        
        "tests/enum_codec.test.cpp"

        "tests/chunked_input_stream.test.cpp"
        "tests/chunked_output_stream.test.cpp"
        "tests/memory_input_stream.test.cpp"
        "tests/memory_output_stream.test.cpp"

        "tests/perfect_hash.test.cpp"
    )

    target_link_libraries(deeppack-tests PRIVATE
        Deeplex::deeppack

        Boost::unit_test_framework
        fmt::fmt
    )

    add_test(NAME deeppack-tests COMMAND deeppack-tests)
endif()


########################################################################
# cmake install
include(GNUInstallDirs)

install(DIRECTORY include/ TYPE INCLUDE)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated/include/ TYPE INCLUDE)
install(TARGETS deeppack EXPORT deeppack-targets)
install(EXPORT deeppack-targets
    NAMESPACE Deeplex::
    DESTINATION "${CMAKE_INSTALL_DATADIR}/deeppack"
)

include(CMakePackageConfigHelpers)
configure_package_config_file(tools/deeppack-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/deeppack-config.cmake"

    INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}/deeppack"

)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/deeppack-config-version.cmake"
    COMPATIBILITY ExactVersion
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/deeppack-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/deeppack-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/deeppack"
)
