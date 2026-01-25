#ifndef TYPES_H
#define TYPES_H

#include <cstdint>

using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;

enum class Side : std::uint8_t {
    BUY,
    SELL,
};

enum class OrderType : std::uint8_t {
    LIMIT,
    CANCEL,
    MARKET,
};

struct Trade {
    OrderId takerOrderId;
    OrderId makerOrderId;
    Price price;
    Quantity quantity;
};

#endif