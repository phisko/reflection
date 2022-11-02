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

    putils_add_test_executable(${test_exe_name} ${test_src})
    target_link_libraries(${test_exe_name} PRIVATE reflection)
endif()