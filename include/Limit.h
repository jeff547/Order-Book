#include "Types.h"

class Order;

class Limit {
private:
    // 8 Bytes
    Order* head = nullptr;
    Order* tail = nullptr;
    // 4 Bytes
    const Price limitPrice_;
    Quantity size_;
    Quantity totalVolume_;

    friend class Order;

public:
    Limit(Price limitPrice, Quantity size = 0, Quantity totalVolume = 0)
        : limitPrice_(limitPrice)
        , size_(size)
        , totalVolume_(totalVolume) {}

    Order* getHead() const { return head; }
    Price getLimitPrice() const { return limitPrice_; }
    Quantity getSize() const { return size_; }
    Quantity getTotalVolume() const { return totalVolume_; }

    void addOrder(Order* order);
};
