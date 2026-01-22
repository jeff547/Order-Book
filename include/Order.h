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
    OrderId orderId_;
    // 4 bytes
    Price price_;
    Quantity qty_;
    // 2 bytes
    OrderType orderType_;
    Side side_;

    friend class Limit;

public:
    Order(OrderId orderId, OrderType orderType, Side side, Price price, Quantity qty)
        : orderId_(orderId)
        , price_(price)
        , qty_(qty)
        , orderType_(orderType)
        , side_(side) {}

    OrderId getOrderId() const { return orderId_; }
    OrderType getOrderType() const { return orderType_; }
    Side getSide() const { return side_; }
    Price getPrice() const { return price_; }
    Quantity getQuantity() const { return qty_; }

    void fill(Quantity qty);
    bool isFilled() const;
};

#endif