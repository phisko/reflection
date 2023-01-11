cmake_minimum_required(VERSION 3.3)
project(putils_reflection HOMEPAGE_URL "https://github.com/phisko/reflection")

set(CMAKE_CXX_STANDARD 20)

add_library(putils_reflection INTERFACE)
target_include_directories(putils_reflection INTERFACE .)

add_subdirectory(meta)
target_link_libraries(putils_reflection INTERFACE putils_meta)

include(generate_reflection_headers.cmake)

option(PUTILS_REFLECTION_TESTS "Build reflection tests" OFF)
if (PUTILS_REFLECTION_TESTS)
    enable_testing()

    set(test_exe_name putils_reflection_tests)
    file(GLOB test_src putils/tests/*.tests.cpp)

    putils_add_test_executable(${test_exe_name} ${test_src})
    target_link_libraries(${test_exe_name} PRIVATE putils_reflection)
endif()