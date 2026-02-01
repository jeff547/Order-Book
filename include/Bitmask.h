#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

class Bitmask {
private:
    std::vector<std::uint64_t> limitPrices;
    size_t size;

public:
    Bitmask(size_t maxSize)
        : limitPrices((maxSize + 63) / 64, 0)
        , size(maxSize) {}

    void set(size_t price) { limitPrices[price / 64] |= (1ULL << (price % 64)); }

    void unset(size_t price) { limitPrices[price / 64] &= ~(1ULL << (price % 64)); }

    long long scanAsc(size_t startPrice) {
        // Scans the bitset ASCENDING order (lowest to highest) (For Asks): Find the lowest cost Ask

        size_t blockIdx = startPrice / 64;
        size_t bitIdx = startPrice % 64;

        // Mask out any prices < startPrice
        uint64_t current = limitPrices[blockIdx] & (~0ULL << bitIdx);

        // Check current block of prices
        if (current != 0) {
            return (blockIdx * 64) + __builtin_ctzll(current);
        }

        // Iterate through each block until you find a valid order
        for (blockIdx++; blockIdx < limitPrices.size(); blockIdx++) {
            if (limitPrices[blockIdx] != 0) {
                return (blockIdx * 64) + __builtin_ctzll(limitPrices[blockIdx]);
            }
        }

        // No asks remaining
        return -1;
    }

    long long scanDesc(size_t startPrice) {
        // Scans the bitset DESCENDING order (highest to lowest) (For Bids): Find the highest cost Bid
        if (startPrice >= size)
            return -1;

        size_t blockIdx = startPrice / 64;
        size_t bitIdx = startPrice % 64;

        // Mask out any prices > startPrice
        std::uint64_t mask = (bitIdx == 63) ? ~0ULL : ~(~0ULL << (bitIdx + 1));
        uint64_t current = limitPrices[blockIdx] & mask;

        // Check current block of prices
        if (current != 0) {
            return (blockIdx * 64) + (63 - __builtin_clzll(current));
        }

        // Iterate through each block until you find a valid order
        for (int i = blockIdx - 1; i >= 0; i--) {
            if (limitPrices[i] != 0) {
                return (i * 64) + (63 - __builtin_clzll(limitPrices[i]));
            }
        }

        // No bids remaining
        return -1;
    }
};
