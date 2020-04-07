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
cmake_minimum_required(VERSION 3.15)
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
if (BUILD_TESTING)
    option(BUILD_TESTING "Add test targets" BUILD_TESTING)
endif()


########################################################################
# dependencies

if (BUILD_TESTING)
    find_package(fmt REQUIRED)

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
elseif (GXX_COMPATIBLE)
    add_compile_options(
        -Wall # easily avoidable code smells
        -Wextra # more code smell warnings
        -pedantic # ISO C++ conformance warnings
        -Werror # treat warnings as errors
        # with the following exceptions
    )
endif()


########################################################################
# library
add_library(deeppack)
add_library(Deeplex::deeppack ALIAS deeppack)
target_include_directories(deeppack PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_sources(deeppack PRIVATE
    src/placeholder.cpp
)
set(DP_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/dplx")
target_sources(deeppack PUBLIC
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/concepts.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/type_code.hpp>
    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/type_encoder.hpp>

    $<BUILD_INTERFACE:${DP_INC_DIR}/dp/utils.hpp>
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

        "tests/test_output_stream.hpp"
        "tests/test_utils.hpp"

        "tests/type_encoder.integer.test.cpp"
        "tests/type_encoder.test.cpp"
      )

    target_link_libraries(deeppack-tests PRIVATE
        Deeplex::deeppack

        Boost::unit_test_framework
        fmt::fmt
    )
    set_target_properties(deeppack-tests PROPERTIES
        CXX_EXTENSIONS OFF
    )

    add_test(NAME deeppack-tests COMMAND deeppack-tests)
endif()


########################################################################
# cmake install
include(GNUInstallDirs)

install(DIRECTORY include/ TYPE INCLUDE)
install(TARGETS deeppack EXPORT deeppack-targets)
install(EXPORT deeppack-targets
    NAMESPACE Deeplex::
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/deeppack"
)

include(CMakePackageConfigHelpers)
configure_file(tools/deeppack-config.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/deeppack-config.cmake"
    COPYONLY
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/deeppack-config-version.cmake"
    COMPATIBILITY ExactVersion
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/deeppack-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/deeppack-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/deeppack"
)
