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