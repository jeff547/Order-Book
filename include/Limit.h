#ifndef LIMIT_H
#define LIMIT_H

#include "Order.h"
#include "Types.h"

struct Order;

struct Limit {
    // Pointers (16 bytes)
    Order* head = nullptr;
    Order* tail = nullptr;
    // Data (12 bytes)
    Price limitPrice;
    Quantity size;
    Quantity totalVolume;

    Limit(Price price)
        : limitPrice(price)
        , size(0)
        , totalVolume(0) {}

    ~Limit() {
        head = nullptr;
        tail = nullptr;
    }

    void addOrder(Order* order) {
        order->parentLimit = this;

        if (head == nullptr) {
            head = tail = order;
        } else {
            tail->nextOrder = order;
            order->prevOrder = tail;
            order->nextOrder = nullptr;
            tail = order;
        }

        size++;
        totalVolume += order->qty;
    }

    void removeOrder(Order* order) {
        // Unlink from Queue
        if (order->prevOrder == nullptr) {
            // Case: Order is Head
            head = order->nextOrder;
        } else {
            order->prevOrder->nextOrder = order->nextOrder;
        }

        if (order->nextOrder == nullptr) {
            // Case: Order is Tail
            tail = order->prevOrder;
        } else {
            order->nextOrder->prevOrder = order->prevOrder;
        }

        // Update Size
        size--;

        // Clean up
        order->nextOrder = nullptr;
        order->prevOrder = nullptr;
        order->parentLimit = nullptr;
    }
};

#endif