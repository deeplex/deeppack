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
cmake_minimum_required(VERSION 3.20)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/tools/cmake/")

########################################################################
# configure vcpkg from environment vars if possible
include(VcpkgDefaults)

if (BUILD_TESTING OR NOT DEFINED BUILD_TESTING)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

########################################################################
project(deeppack
    VERSION 0.0.0.1
    LANGUAGES CXX
)

include(EnforceOutOfSourceBuilds)
include(CMakeDependentOption)
include(CMakePackageConfigHelpers)
include(FeatureSummary)
include(GNUInstallDirs)

include(SourceHelpers)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
enable_testing()


########################################################################
# options
option(DPLX_DP_DISABLE_WORKAROUNDS "Disable all workarounds" OFF)
option(DPLX_DP_FLAG_OUTDATED_WORKAROUNDS "Emit compiler errors for workarounds which are active, but haven't been validated for this version" OFF)
option(DPLX_DP_USE_BRANCHING_INTEGER_ENCODER "Use the branching store_var_uint implementation" OFF)
mark_as_advanced(DPLX_DP_USE_BRANCHING_INTEGER_ENCODER)

cmake_dependent_option(DPLX_DP_TESTS_PCH "enable std header precompilation for test TUs" ON "BUILD_TESTING" OFF)

########################################################################
# dependencies

set(Boost_NO_WARN_NEW_VERSIONS ON)

find_package(concrete CONFIG REQUIRED)

find_package(fmt CONFIG REQUIRED)

find_package(status-code CONFIG REQUIRED)
find_package(outcome CONFIG REQUIRED)

if (BUILD_TESTING)
    set(DP_REQUIRE_UNIT_TEST_FRAMEWORK unit_test_framework)
endif()
find_package(Boost 1.71 REQUIRED COMPONENTS
    ${DP_REQUIRE_UNIT_TEST_FRAMEWORK}
)

find_package(Catch2 CONFIG)
cmake_dependent_option(BUILD_TESTING "Build the documentation using sphinx" ON Catch2_FOUND OFF)
set_package_properties(Catch2 PROPERTIES
    TYPE OPTIONAL
    PURPOSE "Allows to build the test suite"
)

find_package(Sphinx)
cmake_dependent_option(BUILD_DOCS "Build the documentation using sphinx" ON Sphinx_FOUND OFF)
set_package_properties(Sphinx PROPERTIES
    TYPE OPTIONAL
    PURPOSE "Allows to build the documentation"
)


########################################################################
# warning configuration
include(CompilerWarnings)


########################################################################
# additional compiler options
set(CMAKE_CXX_EXTENSIONS OFF CACHE STRING "" FORCE)

add_library(compiler_settings INTERFACE)
add_library(Deeplex::deeppack_compiler_settings ALIAS compiler_settings)
set_target_properties(compiler_settings PROPERTIES
    EXPORT_NAME deeppack_compiler_settings
)

target_compile_features(compiler_settings INTERFACE cxx_std_20)
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(compiler_settings INTERFACE
        /utf-8 # all sources are utf-8 encoded
        /Zc:__cplusplus # correctly define the __cplusplus macro
    )
endif()


########################################################################
# library
add_library(deeppack)
add_library(Deeplex::deeppack ALIAS deeppack)

target_link_libraries(deeppack PUBLIC
    Deeplex::deeppack_compiler_settings

    Boost::boost
    Deeplex::concrete
    fmt::fmt
    outcome::hl
    status-code::hl
)

target_include_directories(deeppack PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated/src>
    $<INSTALL_INTERFACE:include>
)

########################################################################
# library test project
if (BUILD_TESTING)
    add_executable(deeppack-tests)

    target_link_libraries(deeppack-tests PRIVATE
        Deeplex::deeppack

        Catch2::Catch2 Catch2::Catch2WithMain
    )
    
    target_include_directories(deeppack-tests PRIVATE
        src/dp_tests-ng
    )
    
    add_test(NAME deeppack-tests COMMAND deeppack-tests)

    ## legacy tests ####################################################
    add_executable(deeppack-legacy-tests)
    source_group(TREE src/dp_tests PREFIX test_utils)

    target_link_libraries(deeppack-legacy-tests PRIVATE
        Deeplex::deeppack
        
        Boost::unit_test_framework
        fmt::fmt
    )

    target_include_directories(deeppack-legacy-tests PRIVATE
        src/dp_tests
    )

    if (DPLX_DP_TESTS_PCH)
        target_precompile_headers(deeppack-legacy-tests PRIVATE
            <algorithm>
            <array>
            <bit>
            <cmath>
            <concepts>
            <cstddef>
            <cstdint>
            <limits>
            <ranges>
            <span>
            <type_traits>
        )
    endif()

    add_test(NAME deeppack-legacy-tests COMMAND deeppack-legacy-tests)
endif()

########################################################################
# source files
include(sources.cmake)
source_group(TREE src/dplx)


########################################################################
# docs
if (BUILD_DOCS)
    add_subdirectory(docs)
endif()

########################################################################
# cmake install
install(DIRECTORY src/dplx
    TYPE INCLUDE
    FILES_MATCHING
    REGEX ".*\\.h(pp)?"
)

install(TARGETS deeppack compiler_settings EXPORT deeppack-targets)
install(EXPORT deeppack-targets
    NAMESPACE Deeplex::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/deeppack"
)

configure_package_config_file(tools/deeppack-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/deeppack-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/deeppack"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/deeppack-config-version.cmake"
    COMPATIBILITY SameMinorVersion
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/deeppack-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/deeppack-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/deeppack"
)


########################################################################
feature_summary(WHAT PACKAGES_FOUND PACKAGES_NOT_FOUND)
