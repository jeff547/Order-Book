cmake_minimum_required(VERSION 3.14)

project(OrderBook)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

find_package(Threads REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-O3 -march=native -Wall -Wextra)
endif()

enable_testing()
find_package(GTest REQUIRED)

add_subdirectory(src)
add_subdirectory(tests)