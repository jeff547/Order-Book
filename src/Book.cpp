#include "Book.h"
#include <limits>

Book::~Book() {
    orderMap.clear();
    bidsMap.clear();
    asksMap.clear();
}

template <typename BookMap>
void Book::matchOrder(OrderId takerId, BookMap& opposingBook, Price price, Quantity& fillQty, Side side) {
    while (fillQty > 0 && !opposingBook.empty()) {
        // Highest Bid or Lowest Ask
        auto bestLimitIt = opposingBook.begin();
        Limit* bestLimit = bestLimitIt->second;

        // Check if Profitable
        if (side == Side::BUY && bestLimit->limitPrice > price)
            break;
        if (side == Side::SELL && bestLimit->limitPrice < price)
            break;

        // Iterate through Limit Queue
        while (fillQty > 0 && bestLimit->size > 0) {
            Order* headOrder = bestLimit->head;

            // Benchmarking Only
            if (tradeListener) {
                Quantity tradeQty = std::min(fillQty, headOrder->qty);

                tradeListener({
                    takerId,
                    headOrder->orderId,
                    bestLimit->limitPrice,
                    tradeQty,
                });
            }

            if (headOrder->qty > fillQty) {
                // Case A: (Full Fill of Taker's Order)
                headOrder->fill(fillQty);
                bestLimit->totalVolume -= fillQty;
                fillQty = 0;
            }

            else {
                // Case B: (Partial Fill of Taker's Order)
                fillQty -= headOrder->qty;
                // Fully Fill Maker's Order
                headOrder->fill(headOrder->qty);
                // Remove from Order Map
                orderMap.erase(headOrder->orderId);
                // Remove from Limit Queue
                bestLimit->removeOrder(headOrder);
                orderPool.release(headOrder);
            }
        }

        // Remove Limit When Empty
        if (bestLimit->size == 0) {
            limitPool.release(bestLimit);
            opposingBook.erase(bestLimitIt);
        }
    }
}

void Book::addLimitOrder(OrderId id, Price price, Quantity qty, Side side) {
    switch (side) {
    case Side::BUY:
        matchOrder(id, asksMap, price, qty, side);
        break;
    case Side::SELL:
        matchOrder(id, bidsMap, price, qty, side);
        break;
    default:
        __builtin_unreachable();
    }

    // If there are still shares to fill, create a new order
    if (qty > 0) {
        // Create new order and add to Order Lookup Map
        Order* newOrder = orderPool.acquire(id, price, qty, OrderType::LIMIT, side);
        orderMap.emplace(id, newOrder);

        // Add order to respective book
        switch (side) {
        case Side::BUY: { // Get reference to the pointer location (auto-creates nullptr if missing)
            Limit*& limit = bidsMap[price];

            // If it was null, allocate it now
            if (!limit) {
                limit = limitPool.acquire(price);
            }

            limit->addOrder(newOrder);
            break;
        }
        case Side::SELL: {
            Limit*& limit = asksMap[price];

            if (!limit) {
                limit = limitPool.acquire(price);
            }

            limit->addOrder(newOrder);
            break;
        }
        default:
            __builtin_unreachable();
        }
    }
}

void Book::addMarketOrder(OrderId id, Quantity qty, Side side) {
    switch (side) {
    case Side::BUY:
        matchOrder(id, asksMap, std::numeric_limits<Price>::max(), qty, side);
        break;
    case Side::SELL:
        matchOrder(id, bidsMap, std::numeric_limits<Price>::min(), qty, side);
        break;
    default:
        __builtin_unreachable();
    }
}

void Book::cancelOrder(OrderId id) {
    // O(1) Order Lookup
    auto it = orderMap.find(id);
    // Check if order actually exists
    if (it == orderMap.end())
        return;

    Order* order = it->second;
    Limit* parentLimit = order->parentLimit;

    parentLimit->removeOrder(order);

    if (parentLimit->size == 0) {
        if (order->side == Side::BUY) {
            bidsMap.erase(parentLimit->limitPrice);
        } else {
            asksMap.erase(parentLimit->limitPrice);
        }

        limitPool.release(parentLimit);
    }

    orderMap.erase(it);
    orderPool.release(order);
}
