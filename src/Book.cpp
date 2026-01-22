#include "Book.h"

Book::~Book() {
    for (auto const& [key, val] : bids) {
        delete val;
    }
    bids.clear();

    for (auto const& [key, val] : asks) {
        delete val;
    }
    asks.clear();

    for (auto const& [key, val] : orderLookup) {
        delete val;
    }
    orderLookup.clear();
}