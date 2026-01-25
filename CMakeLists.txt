cmake_minimum_required(VERSION 3.14)

project(OrderBook)

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-O3 -march=native -Wall -Wextra)
endif()

enable_testing()
find_package(GTest REQUIRED)

add_subdirectory(src)
add_subdirectory(tests)