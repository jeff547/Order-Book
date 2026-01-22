#include "Order.h"

#include <iostream>

void Order::fill(Quantity fill_qty) {
    if (fill_qty > qty_) {
        throw std::invalid_argument("Order ('" + std::to_string(orderId_) + "') cannot be overfilled");
    }

    qty_ -= fill_qty;
}

bool Order::isFilled() const { return qty_ == 0; }