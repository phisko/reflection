cmake_minimum_required(VERSION 3.0)
project(reflection HOMEPAGE_URL "https://github.com/phisko/reflection")

set(CMAKE_CXX_STANDARD 20)

add_library(reflection INTERFACE)
target_include_directories(reflection INTERFACE .)

add_subdirectory(meta)

option(PUTILS_REFLECTION_TESTS "Build reflection tests" OFF)
if (PUTILS_REFLECTION_TESTS)
    enable_testing()

    set(test_exe_name putils_reflection_tests)
    file(GLOB test_src tests/*.tests.cpp)

    add_executable(${test_exe_name} ${test_src})
    target_link_libraries(${test_exe_name} PRIVATE reflection)

    set(customFindPackageNames gtest:GTest)
    set(customLibraryNames gtest:GTest)
    putils_conan_download_and_link_packages_with_names(
        ${test_exe_name} PRIVATE
        "${customFindPackageNames}"
        "${customLibraryNames}"
        gtest/cci.20210126
    )

    include(GoogleTest)
    gtest_discover_tests(${test_exe_name})
endif()