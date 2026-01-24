add_executable(OrderBookTests 
    OrderBookTests.cpp
)

target_link_libraries(OrderBookTests 
    GTest::gtest_main 
    OrderBookCore
)