cmake_minimum_required(VERSION 3.20)
project(cache LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(GTest 1.14 CONFIG REQUIRED)

# FIXME: instead of writing target_include_directories twice you may create an interface library
# out of all you headers and then link with it twice.
# https://cmake.org/cmake/help/latest/command/add_library.html#interface-libraries
add_library(cache_headers INTERFACE)
target_include_directories(cache_headers INTERFACE "include")

add_executable(cache src/cache.cpp)
target_link_libraries(cache PRIVATE cache_headers)

add_executable(cache_tests
    test/common_tests.cpp
    test/twoq_tests.cpp
    test/arc_tests.cpp
)

target_link_libraries(cache_tests PRIVATE
    cache_headers
    GTest::gtest_main
    Threads::Threads
)

enable_testing()

# FIXME: why not use gtest_discover_tests? Now when you run ctest you see only one test passing, but
# in fact there are many

include(GoogleTest)
gtest_discover_tests(cache_tests)

message(STATUS "GoogleTest version: ${GTest_VERSION}")
message(STATUS "GoogleTest config: ${GTest_DIR}")

get_target_property(GTEST_INCLUDE_PATHS GTest::gtest INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "GoogleTest headers: ${GTEST_INCLUDE_PATHS}")