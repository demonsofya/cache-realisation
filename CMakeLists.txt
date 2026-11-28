cmake_minimum_required(VERSION 3.20)
project(cache LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
include(FetchContent)
option(CACHE_FETCH_DEPENDENCIES "Download missing CLI11 and GoogleTest packages" ON)

find_package(CLI11 2.4 CONFIG QUIET)
if(NOT CLI11_FOUND)
    if(NOT CACHE_FETCH_DEPENDENCIES)
        message(FATAL_ERROR "CLI11 is required; install it or enable CACHE_FETCH_DEPENDENCIES")
    endif()
    FetchContent_Declare(CLI11
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG 6c7b07a878ad834957b98d0f9ce1dbe0cb204fc9
    )
    FetchContent_MakeAvailable(CLI11)
endif()

add_library(cache_headers INTERFACE)
target_include_directories(cache_headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(cache_core src/cache_hierarchy.cpp src/cache_config.cpp src/ideal_cache.cpp)
target_link_libraries(cache_core PUBLIC cache_headers)

add_executable(cache_runner src/cache_runner.cpp)
target_link_libraries(cache_runner PRIVATE cache_core CLI11::CLI11)

add_executable(cache src/cache.cpp)
target_link_libraries(cache PRIVATE cache_core)

if(BUILD_TESTING)
    find_package(Threads REQUIRED)
    find_package(GTest 1.14 CONFIG QUIET)
    if(NOT GTest_FOUND)
        if(NOT CACHE_FETCH_DEPENDENCIES)
            message(FATAL_ERROR "GoogleTest is required for tests; install it or disable BUILD_TESTING")
        endif()
        FetchContent_Declare(googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG f8d7d77c06936315286eb55f8de22cd23c188571
        )
        FetchContent_MakeAvailable(googletest)
    endif()
    add_executable(cache_tests
        test/common_tests.cpp
        test/twoq_tests.cpp
        test/arc_tests.cpp
        test/ideal_tests.cpp
        test/hierarchy_tests.cpp
        test/config_tests.cpp
    )
    target_link_libraries(cache_tests PRIVATE cache_core GTest::gtest_main Threads::Threads)
    include(GoogleTest)
    gtest_discover_tests(cache_tests)

    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME RunnerIntegration
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_cache_runner.py"
                    --binary $<TARGET_FILE:cache_runner>)
    endif()
endif()
