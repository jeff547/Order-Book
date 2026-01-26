#ifndef ORDER_H
#define ORDER_H

#include "Types.h"

struct Limit;

struct Order {

    // Pointers (8 bytes each = 24 bytes)
    Order* nextOrder = nullptr;
    Order* prevOrder = nullptr;
    Limit* parentLimit = nullptr;
    // Data (8 bytes + 4 bytes + 4 bytes = 16 bytes)
    OrderId orderId;
    Price price;
    Quantity qty;
    // Enums (1 byte each + padding = 2-8 bytes)
    OrderType orderType;
    Side side;

    Order(OrderId id, Price p, Quantity q, OrderType type, Side s)
        : orderId(id)
        , price(p)
        , qty(q)
        , orderType(type)
        , side(s) {}

    void fill(Quantity fillQty) { qty -= fillQty; }
};

#endif