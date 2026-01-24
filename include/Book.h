#ifndef BOOK_H
#define BOOK_H

#include "Limit.h"
#include "Order.h"

#include <map>
#include <unordered_map>

class Book {
private:
    // Bids (Buys): Ordered High-to-Low (Highest bidder is best)
    std::map<Price, Limit*, std::greater<Price>> bidsMap;
    // Asks (Sells): Ordered Low-to-High (Lowest seller is best)
    std::map<Price, Limit*, std::less<Price>> asksMap;
    // Fast Lookup: Hash Map OrderID -> Order Object
    std::unordered_map<OrderId, Order*> orderMap;

    template <typename BookMap> void matchOrder(BookMap& opposingBook, Price price, Quantity& quantity, Side side);

    friend class OrderBookTest;

public:
    ~Book();

    void addLimitOrder(OrderId orderId, Price price, Quantity quantity, Side side);
    void addMarketOrder(OrderId orderId, Quantity quantity, Side side);
    void cancelOrder(OrderId orderId);
};

#endif