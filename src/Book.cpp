#include "Book.h"
#include <limits>

Book::~Book() { orderMap.clear(); }

void Book::updateBestAsk() {
    long long next = asksMask.scanAsc(lowestAsk);
    lowestAsk = (next == -1) ? MAX_PRICE : static_cast<Price>(next);
}

void Book::updateBestBid() {
    long long next = bidsMask.scanDesc(highestBid);
    highestBid = (next == -1) ? 0 : static_cast<Price>(next);
}

void Book::matchOrder(OrderId takerId, Price price, Quantity& fillQty, Side side) {
    auto& opposingBook = (side == Side::BUY) ? asks : bids;
    Price* bestPrice = (side == Side::BUY) ? &lowestAsk : &highestBid;
    Bitmask& opposingBookMask = (side == Side::BUY) ? asksMask : bidsMask;

    while (fillQty > 0) {
        // Check if book is empty
        if (side == Side::BUY && *bestPrice >= MAX_PRICE)
            break;
        else if (side == Side::SELL && *bestPrice == 0)
            break;
        // Check if Profitable
        if (side == Side::BUY && *bestPrice > price)
            break;
        else if (side == Side::SELL && *bestPrice < price)
            break;

        Limit* bestLimit = opposingBook[*bestPrice];
        // No orders at the limit
        if (bestLimit == nullptr) {
            opposingBookMask.unset(*bestPrice);
            if (side == Side::BUY) {
                updateBestAsk();
            } else {
                updateBestBid();
            }
            continue;
        }

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
                orderMap[headOrder->orderId] = nullptr;
                // Remove from Limit Queue
                bestLimit->removeOrder(headOrder);
                orderPool.release(headOrder);
            }
        }

        // Remove Limit When Empty
        if (bestLimit->size == 0) {
            limitPool.release(bestLimit);
            opposingBook[*bestPrice] = nullptr;
            opposingBookMask.unset(*bestPrice);
            if (side == Side::BUY) {
                updateBestAsk();
            } else {
                updateBestBid();
            }
        }
    }
}

void Book::addLimitOrder(OrderId id, Price price, Quantity qty, Side side) {
    matchOrder(id, price, qty, side);

    // If there are still shares to fill, create a new order
    if (qty > 0) {
        // Create new order and add to Order Lookup Map
        Order* newOrder = orderPool.acquire(id, price, qty, OrderType::LIMIT, side);
        orderMap[id] = newOrder;

        // Get respective book and bookMask
        auto& book = (side == Side::BUY) ? bids : asks;
        auto& mask = (side == Side::BUY) ? bidsMask : asksMask;

        // Get Limit or create one if it doesn't exist
        Limit*& limit = book[price];

        if (!limit) {
            limit = limitPool.acquire(price);
            mask.set(price);

            if (side == Side::BUY && price > highestBid) {
                highestBid = price;
            } else if (side == Side::SELL && price < lowestAsk) {
                lowestAsk = price;
            }
        }
        // Add the new Order to Limit
        limit->addOrder(newOrder);
    }
}

void Book::addMarketOrder(OrderId id, Quantity qty, Side side) {
    if (side == Side::BUY) {
        matchOrder(id, std::numeric_limits<Price>::max(), qty, side);
    } else {
        matchOrder(id, std::numeric_limits<Price>::min(), qty, side);
    }
}

void Book::cancelOrder(OrderId id) {
    // Check if order actually exists
    Order* order = orderMap[id];
    if (order == nullptr)
        return;

    Limit* parentLimit = order->parentLimit;
    parentLimit->removeOrder(order);

    if (parentLimit->size == 0) {
        Price p = parentLimit->limitPrice;
        limitPool.release(parentLimit);

        if (order->side == Side::BUY) {
            bids[p] = nullptr;
            bidsMask.unset(p);
            if (p == highestBid) {
                updateBestBid();
            }
        } else {
            asks[p] = nullptr;
            asksMask.unset(p);
            if (p == lowestAsk) {
                updateBestAsk();
            }
        }
    }

    orderMap[id] = nullptr;
    orderPool.release(order);
}
