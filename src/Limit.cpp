#include "Limit.h"
#include "Order.h"

void Limit::addOrder(Order* order) {
    if (head == nullptr) {
        head = tail = order;
    } else {
        tail->nextOrder = order;
        order->prevOrder = tail;
        order->nextOrder = nullptr;
        tail = order;
    }

    size_++;
    totalVolume_ += order->getQuantity();
    order->parentLimit = this;
}