#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>

using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;

enum class Side : std::uint8_t {
    BUY,
    SELl,
};

enum class OrderType : std::uint8_t {
    MARKET,
    LIMIT,
};

#endif