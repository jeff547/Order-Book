#include "Order.h"
#include "Limit.h"

#include <iostream>

void Order::partialFill(Quantity fill_qty) {
    if (fill_qty > qty_) {
        throw std::invalid_argument("Order ('" + std::to_string(orderId_) + "') cannot be overfilled");
    }

    qty_ -= fill_qty;
}

void Order::removeSelf() {
    // Unlink from Queue
    if (prevOrder == nullptr) {
        // Order is Head
        parentLimit->head = nextOrder;
    } else {
        prevOrder->nextOrder = nextOrder;
    }

    if (nextOrder == nullptr) {
        // Order is Tail
        parentLimit->tail = prevOrder;
    } else {
        nextOrder->prevOrder = prevOrder;
    }

    // Update Limit State
    parentLimit->size_--;
    parentLimit->totalVolume_ -= qty_;

    // Clean up
    nextOrder = nullptr;
    prevOrder = nullptr;
    parentLimit = nullptr;
}

bool Order::isFilled() const { return qty_ == 0; }
