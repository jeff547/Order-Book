#ifndef BOOK_H
#define BOOK_H

#include "Limit.h"
#include "Order.h"

#include <map>
#include <unordered_map>

class Book {
private:
    // Bids (Buys): Ordered High-to-Low (Highest bidder is best)
    std::map<Price, Limit*, std::greater<Price>> bids;
    // Asks (Sells): Ordered Low-to-High (Lowest seller is best)
    std::map<Price, Limit*, std::less<Price>> asks;
    // Fast Lookup: Hash Map OrderID -> Order Object
    std::unordered_map<OrderId, Order*> orderLookup;

public:
    Book();
    ~Book();

    void addOrder(OrderId orderId, double price, int quantity, OrderType type, Side side);
    void cancelOrder(OrderId orderId);

    void printBook();
};

#endif