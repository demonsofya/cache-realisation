cmake_minimum_required(VERSION 3.20)
project(cache LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

# FIXME: instead of writing target_include_directories twice you may create an interface library
# out of all you headers and then link with it twice.
# https://cmake.org/cmake/help/latest/command/add_library.html#interface-libraries

# FIXME: ${CMAKE_CURRENT_SOURCE_DIR} is redundant, paths are relative to the directory of current
# CMakeLists.txt by default
add_executable(cache src/cache.cpp)
target_include_directories(cache PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(cache_tests test/test.cpp)
# FIXME: same as above
target_include_directories(cache_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(cache_tests PRIVATE
    GTest::gtest
    Threads::Threads
)

enable_testing()

# FIXME: why not use gtest_discover_tests? Now when you run ctest you see only one test passing, but
# in fact there are many
add_test(NAME cache_tests COMMAND cache_tests)
