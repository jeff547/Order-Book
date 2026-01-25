add_library(OrderBookCore STATIC
    Book.cpp
    Order.cpp
    Limit.cpp
    ../include/Book.h
    ../include/Order.h
    ../include/Limit.h
)

target_include_directories(OrderBookCore PUBLIC ../include)

add_executable(OrderBookApp main.cpp)
target_link_libraries(OrderBookApp PRIVATE OrderBookCore)

add_executable(run_benchmark Benchmark.cpp)
target_link_libraries(run_benchmark PRIVATE OrderBookCore)

if(MSVC)
    target_compile_options(run_benchmark PRIVATE /O2 /Ob2)
else()
    target_compile_options(run_benchmark PRIVATE -O3 -march=native)
endif()

if(UNIX)
    target_link_libraries(run_benchmark PRIVATE pthread)
endif()