#include "constants.h"

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
    Limit(Price limitPrice, Quantity size_ = 0, Quantity totalVolume_ = 0)
        : limitPrice_(limitPrice) {}

    Order* getHead() const { return head; }
    const Price getLimitPrice() const { return limitPrice_; }
    const Quantity getSize() const { return size_; }
    const Quantity getTotalVolume() const { return totalVolume_; }

    void addOrder(Order* order);
};
