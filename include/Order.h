#ifndef ORDER_H
#define ORDER_H

#include "constants.h"

class Limit;

class Order {

private:
    // 8 bytes
    Order* nextOrder = nullptr;
    Order* prevOrder = nullptr;
    Limit* parentLimit = nullptr;
    const OrderId orderId_;
    // 4 bytes
    const Price price_;
    Quantity qty_;
    // 2 bytes
    const OrderType orderType_;
    const Side side_;

    friend class Limit;

public:
    Order(OrderId orderId, Price price, Quantity qty, OrderType orderType, Side side)
        : orderId_(orderId)
        , price_(price)
        , qty_(qty)
        , orderType_(orderType)
        , side_(side) {}

    const OrderId getOrderId() const { return orderId_; }
    const OrderType getOrderType() const { return orderType_; }
    const Side getSide() const { return side_; }
    const Price getPrice() const { return price_; }
    Quantity getQuantity() const { return qty_; }
    Limit* getParentLimit() const { return parentLimit; }
    const Order* getNext() const { return nextOrder; }
    const Order* getPrev() const { return prevOrder; }

    void partialFill(Quantity qty);
    void removeSelf();
    bool isFilled() const;
};

#endif