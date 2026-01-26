#include "Book.h"
#include <limits>

Book::~Book() {
    for (auto const& [key, val] : orderMap) {
        delete val;
    }
    orderMap.clear();

    for (auto const& [key, val] : bidsMap) {
        delete val;
    }
    bidsMap.clear();

    for (auto const& [key, val] : asksMap) {
        delete val;
    }
    asksMap.clear();
}

template <typename BookMap>
void Book::matchOrder(OrderId incomingOrderId, BookMap& opposingBook, Price orderLimitPrice, Quantity& fillQty,
                      Side side) {
    while (fillQty > 0 && !opposingBook.empty()) {
        // Highest Bid or Lowest Ask
        auto bestLimitIt = opposingBook.begin();
        Limit* bestLimit = bestLimitIt->second;

        // Check if Profitable
        if (side == Side::BUY && bestLimit->limitPrice > orderLimitPrice)
            break;
        if (side == Side::SELL && bestLimit->limitPrice < orderLimitPrice)
            break;

        // Iterate through Limit Queue
        while (fillQty > 0 && bestLimit->size > 0) {
            Order* headOrder = bestLimit->head;

            if (tradeListener) {
                Quantity tradeQty = std::min(fillQty, headOrder->qty);

                tradeListener({
                    incomingOrderId,
                    headOrder->orderId,
                    bestLimit->limitPrice,
                    tradeQty,
                });
            }

            if (headOrder->qty > fillQty) {
                // Case A: (Full Fill of Incoming Order)
                headOrder->fill(fillQty);
                bestLimit->totalVolume -= headOrder->qty;
                fillQty = 0;
            }

            else {
                // Case B: (Partial Fill of Incoming Order)
                fillQty -= headOrder->qty;
                // Fully Fill Takers Order
                headOrder->fill(headOrder->qty);
                // Remove from Order Map
                orderMap.erase(headOrder->orderId);
                // Remove from Limit Queue
                bestLimit->removeOrder(headOrder);
                delete headOrder;
            }
        }

        // Remove Limit When Empty
        if (bestLimit->size == 0) {
            delete bestLimit;
            opposingBook.erase(bestLimitIt);
        }
    }
}

void Book::addLimitOrder(OrderId orderId, Price orderLimitPrice, Quantity fillQty, Side side) {
    switch (side) {
    case Side::BUY:
        matchOrder(orderId, asksMap, orderLimitPrice, fillQty, side);
        break;
    case Side::SELL:
        matchOrder(orderId, bidsMap, orderLimitPrice, fillQty, side);
        break;
    default:
        __builtin_unreachable();
    }

    // If there are still shares to fill, create a new order
    if (fillQty > 0) {
        // Create new order and add to Order Lookup Map
        Order* newOrder = new Order(orderId, orderLimitPrice, fillQty, OrderType::LIMIT, side);
        orderMap.emplace(orderId, newOrder);

        // Add order to respective book
        switch (side) {
        case Side::BUY: { // Get reference to the pointer location (auto-creates nullptr if missing)
            Limit*& limit = bidsMap[orderLimitPrice];

            // If it was null, allocate it now
            if (!limit)
                limit = new Limit(orderLimitPrice);

            limit->addOrder(newOrder);
            break;
        }
        case Side::SELL: {
            Limit*& limit = asksMap[orderLimitPrice];

            if (!limit)
                limit = new Limit(orderLimitPrice);

            limit->addOrder(newOrder);
            break;
        }
        default:
            __builtin_unreachable();
        }
    }
}

void Book::addMarketOrder(OrderId orderId, Quantity quantity, Side side) {
    switch (side) {
    case Side::BUY:
        matchOrder(orderId, asksMap, std::numeric_limits<Price>::max(), quantity, side);
        break;
    case Side::SELL:
        matchOrder(orderId, bidsMap, std::numeric_limits<Price>::min(), quantity, side);
        break;
    default:
        __builtin_unreachable();
    }
}

void Book::cancelOrder(OrderId orderId) {
    // O(1) Order Lookup
    auto it = orderMap.find(orderId);
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

        delete parentLimit;
    }

    orderMap.erase(it);
    delete order;
}
