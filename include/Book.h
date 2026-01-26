#ifndef BOOK_H
#define BOOK_H

#include "Limit.h"
#include "ObjectPool.h"
#include "Order.h"

#include <functional>
#include <map>
#include <unordered_map>

using TradeCallback = std::function<void(const Trade&)>;

class Book {
private:
    // Bids (Buys): Ordered High-to-Low (Highest bidder is best)
    std::map<Price, Limit*, std::greater<Price>> bidsMap;
    // Asks (Sells): Ordered Low-to-High (Lowest seller is best)
    std::map<Price, Limit*, std::less<Price>> asksMap;
    // Fast Lookup: Hash Map OrderID -> Order Object
    std::unordered_map<OrderId, Order*> orderMap;

    // Object pools
    ObjectPool<Order> orderPool;
    ObjectPool<Limit> limitPool;

    // For Benchmarking (Observer)
    TradeCallback tradeListener = nullptr;

    template <typename BookMap>
    void matchOrder(OrderId takerId, BookMap& opposingBook, Price price, Quantity& fillQty, Side side);

    friend class OrderBookTest;

public:
    Book(size_t maxOrders, size_t maxLimits)
        : orderPool(maxOrders)
        , limitPool(maxLimits) {}

    ~Book();

    void addLimitOrder(OrderId id, Price price, Quantity qty, Side side);
    void addMarketOrder(OrderId id, Quantity qty, Side side);
    void cancelOrder(OrderId id);

    // For Benchmarking
    void setTradeCallback(const TradeCallback& cb) { tradeListener = cb; }
};

#endif