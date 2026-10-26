cmake_minimum_required(VERSION 3.28)

project(cache)

set(CMAKE_CXX_STANDARD 17)

add_executable(cache cache.cpp cache.hpp)

add_executable(cache_tests test.cpp cache.hpp)

target_link_libraries(cache_tests PRIVATE GTest::gtest)

enable_testing()
add_test(NAME cache_tests COMMAND cache_tests)
