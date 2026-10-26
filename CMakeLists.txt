cmake_minimum_required(VERSION 3.10)
project(cache)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(../googletest-release-1.8.0/googletest googletest)

add_executable(cache_tests test.cpp)

target_link_libraries(cache_tests gtest pthread)