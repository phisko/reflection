set(CMAKE_CXX_STANDARD 20)

add_library(reflection INTERFACE)
target_include_directories(reflection INTERFACE .)

add_subdirectory(meta)

if (PUTILS_TESTS)
    set(test_exe_name putils_reflection_tests)
    file(GLOB test_src tests/*.tests.cpp)

    add_executable(${test_exe_name} ${test_src})

    putils_conan(gtest/cci.20210126)
    target_link_libraries(${test_exe_name} CONAN_PKG::gtest reflection)

    include(GoogleTest)
    gtest_discover_tests(${test_exe_name})
endif()