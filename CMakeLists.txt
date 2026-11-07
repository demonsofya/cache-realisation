cmake_minimum_required(VERSION 3.20)
project(cache LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GTEST_ROOT_PATH "" CACHE PATH "Путь к папке с исходниками GoogleTest")

if(NOT GTEST_ROOT_PATH)
    message(FATAL_ERROR
        "Укажите путь к GoogleTest через -DGTEST_ROOT_PATH=/путь/к/googletest"
    )
endif()

if(NOT EXISTS "${GTEST_ROOT_PATH}/googletest/CMakeLists.txt")
    message(FATAL_ERROR
        "В ${GTEST_ROOT_PATH}/googletest не найден CMakeLists.txt"
    )
endif()

add_subdirectory("${GTEST_ROOT_PATH}/googletest" googletest)

find_package(Threads REQUIRED)

add_executable(cache src/cache.cpp)
target_include_directories(cache PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(cache_tests test/test.cpp)
target_include_directories(cache_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(cache_tests PRIVATE gtest Threads::Threads)

enable_testing()
add_test(NAME cache_tests COMMAND cache_tests)
add_test(NAME cache_tests COMMAND cache_tests)