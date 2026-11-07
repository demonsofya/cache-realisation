cmake_minimum_required(VERSION 3.20)
project(cache LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

add_executable(cache src/cache.cpp)
target_include_directories(cache PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(cache_tests test/test.cpp)
target_include_directories(cache_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(cache_tests PRIVATE
    GTest::gtest
    Threads::Threads
)

enable_testing()
add_test(NAME cache_tests COMMAND cache_tests)