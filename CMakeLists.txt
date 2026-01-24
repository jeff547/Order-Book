cmake_minimum_required(VERSION 3.14)
project(OrderBook)

set(CMAKE_CXX_STANDARD 20)

find_package(GTest REQUIRED)

add_subdirectory(src)
add_subdirectory(tests)