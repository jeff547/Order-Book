#include "constants.h"

class Order;

class Limit {
private:
    // 8 Bytes
    Order* head = nullptr;
    Order* tail = nullptr;
    // 4 Bytes
    Price price_;
    Quantity size_;
    Quantity volume_;

    friend class Order;

public:
    Limit(Price limitPrice, Quantity _size = 0, Quantity volume_ = 0)
        : price_(limitPrice) {}

    Order* getHead() const { return head; }
    Price getPrice() const { return price_; }
    Quantity getSize() const { return size_; }
    Quantity getVolume() const { return volume_; }

    void enqueue(Order* order);
};
