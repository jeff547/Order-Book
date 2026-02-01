#ifndef BOOK_H
#define BOOK_H

#include "Bitmask.h"
#include "Limit.h"
#include "ObjectPool.h"
#include "Order.h"

#include <functional>

using TradeCallback = std::function<void(const Trade&)>;

class Book {
private:
    // Bids (Buys): Ordered High-to-Low (Highest bidder is best)
    std::vector<Limit*> bids;
    // Asks (Sells): Ordered Low-to-High (Lowest seller is best)
    std::vector<Limit*> asks;

    Bitmask bidsMask;
    Bitmask asksMask;

    Price highestBid = 0;
    Price lowestAsk = MAX_PRICE;

    // Fast Lookup: Hash Map OrderID -> Order Object
    std::vector<Order*> orderMap;

    // Object pools
    ObjectPool<Order> orderPool;
    ObjectPool<Limit> limitPool;

    // For Benchmarking (Observer)
    TradeCallback tradeListener = nullptr;

    void updateBestBid();
    void updateBestAsk();
    void matchOrder(OrderId makerId, Price price, Quantity& fillQty, Side side);

    friend class OrderBookTest;

public:
    Book(size_t maxOrders)
        : bids(MAX_PRICE, nullptr)
        , asks(MAX_PRICE, nullptr)
        , bidsMask(MAX_PRICE)
        , asksMask(MAX_PRICE)
        , orderMap(maxOrders, nullptr)
        , orderPool(maxOrders)
        , limitPool(MAX_PRICE) {}

    ~Book();

    void addLimitOrder(OrderId id, Price price, Quantity qty, Side side);
    void addMarketOrder(OrderId id, Quantity qty, Side side);
    void cancelOrder(OrderId id);

    // For Benchmarking
    void setTradeCallback(const TradeCallback& cb) { tradeListener = cb; }
};

#endif