#include "Book.h"
#include <gtest/gtest.h>

class OrderBookTest : public ::testing::Test {
protected:
    Book book{100000, 1000};

    // ==========================================
    // INSPECTOR METHODS (Helpers)
    // ==========================================

    // Checks if an order ID exists
    bool hasOrder(OrderId id) const { return book.orderMap.contains(id); }

    // Retrieves a pointer to an order
    Order* getOrder(OrderId id) const {
        auto it = book.orderMap.find(id);
        return (it != book.orderMap.end()) ? it->second : nullptr;
    }

    // Returns number of active Price Levels on the Sell side
    size_t getAskDepth() const { return book.asksMap.size(); }

    // Returns number of active Price Levels on the Buy side
    size_t getBidDepth() const { return book.bidsMap.size(); }
};

// =====================================================================
// SECTION 1: PLACEMENT & STATE
// Verify orders rest in the book correctly when no match is possible.
// =====================================================================

TEST_F(OrderBookTest, AddLimitOrder_CreatesNewLevel) {
    // Scenario: Add a single Sell order
    book.addLimitOrder(1, 100, 100, Side::SELL);

    // Expectation:
    // 1. Order exists in lookup
    // 2. Ask side has 1 level
    // 3. Bid side is empty
    ASSERT_TRUE(hasOrder(1));
    EXPECT_EQ(getAskDepth(), 1);
    EXPECT_EQ(getBidDepth(), 0);
}

TEST_F(OrderBookTest, NoMatch_PriceMismatch) {
    // Scenario: Bid is LOWER than Ask (Normal Market State)
    book.addLimitOrder(1, 101, 100, Side::SELL); // Selling at $101
    book.addLimitOrder(2, 100, 100, Side::BUY);  // Buying at $100

    // Expectation: No trade. Spread is $1.
    EXPECT_TRUE(hasOrder(1));
    EXPECT_TRUE(hasOrder(2));
    EXPECT_EQ(getAskDepth(), 1);
    EXPECT_EQ(getBidDepth(), 1);
}

// =====================================================================
// SECTION 2: MATCHING LOGIC (LIMIT ORDERS)
// Verify exact matches and partial fills work bi-directionally.
// =====================================================================

TEST_F(OrderBookTest, FullMatch_RemovesBothOrders) {
    // Scenario: Perfect match. 100 shares vs 100 shares.
    book.addLimitOrder(1, 100, 100, Side::SELL);
    book.addLimitOrder(2, 100, 100, Side::BUY);

    // Expectation: Both orders are fully filled and removed.
    EXPECT_FALSE(hasOrder(1));
    EXPECT_FALSE(hasOrder(2));
    EXPECT_EQ(getAskDepth(), 0);
    EXPECT_EQ(getBidDepth(), 0);
}

TEST_F(OrderBookTest, PartialFill_IncomingIsLarger) {
    // Scenario: Aggressor (Buyer) is larger than Resting (Seller)
    // Sell 50 @ $100
    book.addLimitOrder(1, 100, 50, Side::SELL);

    // Buy 100 @ $100
    book.addLimitOrder(2, 100, 100, Side::BUY);

    // Expectation:
    // 1. Seller (#1) is fully filled (Gone)
    // 2. Buyer (#2) is partially filled (Remains with 50)
    EXPECT_FALSE(hasOrder(1));
    ASSERT_TRUE(hasOrder(2));
    EXPECT_EQ(getOrder(2)->qty, 50);
}

TEST_F(OrderBookTest, PartialFill_RestingIsLarger) {
    // Scenario: Aggressor (Buyer) is smaller than Resting (Seller)
    // Sell 100 @ $100
    book.addLimitOrder(1, 100, 100, Side::SELL);

    // Buy 25 @ $100
    book.addLimitOrder(2, 100, 25, Side::BUY);

    // Expectation:
    // 1. Buyer (#2) is fully filled (Gone)
    // 2. Seller (#1) is partially filled (Remains with 75)
    EXPECT_FALSE(hasOrder(2));
    ASSERT_TRUE(hasOrder(1));
    EXPECT_EQ(getOrder(1)->qty, 75);
}

TEST_F(OrderBookTest, PartialFill_DoesNotChangeQueueOrder) {
    book.addLimitOrder(1, 100, 10, Side::SELL);
    book.addLimitOrder(2, 100, 10, Side::SELL);

    book.addLimitOrder(3, 100, 5, Side::BUY);

    Order* first = getOrder(1);
    Order* second = getOrder(2);

    EXPECT_EQ(first->qty, 5);
    EXPECT_EQ(first->nextOrder, second);
    EXPECT_EQ(second->prevOrder, first);
}

// =====================================================================
// SECTION 3: PRIORITY (PRICE-TIME)
// Verify the engine respects the "Queue" mechanism.
// =====================================================================

TEST_F(OrderBookTest, PriceTimePriority) {
    // Scenario: Two sellers at the SAME price.
    // Order #1 arrives at T=0
    book.addLimitOrder(1, 100, 10, Side::SELL);
    // Order #2 arrives at T=1
    book.addLimitOrder(2, 100, 10, Side::SELL);

    // Aggressor buys 15 shares
    book.addLimitOrder(3, 100, 15, Side::BUY);

    // Expectation:
    // 1. Order #1 (Front of Queue) is fully eaten (10 shares).
    // 2. Order #2 (Back of Queue) gives up 5 shares, keeps 5.
    EXPECT_FALSE(hasOrder(1));

    ASSERT_TRUE(hasOrder(2));
    EXPECT_EQ(getOrder(2)->qty, 5);
}

// =====================================================================
// SECTION 4: MARKET ORDERS (SWEEPING)
// Verify orders that walk the book across multiple price levels.
// =====================================================================

TEST_F(OrderBookTest, MarketBuy_SweepsLevels) {
    // Scenario: Thin liquidity across 3 levels.
    book.addLimitOrder(1, 100, 10, Side::SELL); // Best Price
    book.addLimitOrder(2, 101, 10, Side::SELL); // Mid Price
    book.addLimitOrder(3, 102, 10, Side::SELL); // Worst Price

    // Action: Market Buy 25 shares
    // Math: Takes 10 (@100) + 10 (@101) + 5 (@102) = 25 total
    book.addMarketOrder(4, 25, Side::BUY);

    // Expectation:
    EXPECT_FALSE(hasOrder(1)); // Filled
    EXPECT_FALSE(hasOrder(2)); // Filled

    ASSERT_TRUE(hasOrder(3)); // Survivor
    EXPECT_EQ(getOrder(3)->qty, 5);
}

TEST_F(OrderBookTest, MarketSell_SweepsBids_HighToLow) {
    // Scenario: Buyers at $102 (Best), $101, $100 (Worst)
    book.addLimitOrder(1, 102, 10, Side::BUY);
    book.addLimitOrder(2, 101, 10, Side::BUY);
    book.addLimitOrder(3, 100, 10, Side::BUY);

    // Action: Market Sell 25 shares
    // Should hit #1 ($102) first, then #2, then #3
    book.addMarketOrder(4, 25, Side::SELL);

    // Expectation
    EXPECT_FALSE(hasOrder(1)); // $102 Filled (Best Bid)
    EXPECT_FALSE(hasOrder(2)); // $101 Filled

    ASSERT_TRUE(hasOrder(3)); // $100 Survivor
    EXPECT_EQ(getOrder(3)->qty, 5);
}

TEST_F(OrderBookTest, MarketBuy_ExceedsLiquidity) {
    // Scenario: Only 10 shares available
    book.addLimitOrder(1, 100, 10, Side::SELL);

    // Action: Buy 50 shares (Market)
    // Should fill 10, and the remaining 40 should typically be killed (IOC)
    // or sit as a resting order (depending on your logic).
    // Assuming Standard "Fill and Kill" behavior for Market Orders:

    book.addMarketOrder(2, 50, Side::BUY);

    // Expectation:
    EXPECT_FALSE(hasOrder(1));   // Seller eaten
    EXPECT_FALSE(hasOrder(2));   // Market Order shouldn't rest in book
    EXPECT_EQ(getAskDepth(), 0); // Book is empty
}

// =====================================================================
// SECTION 5: CANCELLATIONS
// Verify orders can be withdrawn before execution.
// =====================================================================

// tests/OrderBookTests.cpp

TEST_F(OrderBookTest, LinkedList_PointersAreConsistent) {
    // 1. Setup
    book.addLimitOrder(1, 100, 10, Side::SELL); // A
    book.addLimitOrder(2, 100, 10, Side::SELL); // B
    book.addLimitOrder(3, 100, 10, Side::SELL); // C

    Order* orderA = getOrder(1);
    Order* orderB = getOrder(2);
    Order* orderC = getOrder(3);

    // 2. Verify Initial State (A <-> B <-> C)
    // Use .getPrev() instead of ->prevOrder
    EXPECT_EQ(orderB->prevOrder, orderA);
    EXPECT_EQ(orderB->nextOrder, orderC);

    // 3. ACTION: Delete Middle (B)
    book.cancelOrder(2);

    // 4. Verify Stitching (A <-> C)
    // A should skip B and point to C
    EXPECT_EQ(orderA->nextOrder, orderC);

    // C should skip B and point back to A
    EXPECT_EQ(orderC->prevOrder, orderA);
}

TEST_F(OrderBookTest, DeleteHead_UpdatesLimitPointer) {
    // Setup: A -> B
    book.addLimitOrder(1, 100, 10, Side::SELL);
    book.addLimitOrder(2, 100, 10, Side::SELL);

    // Action: Delete A
    book.cancelOrder(1);

    // Verification
    Order* orderB = getOrder(2);
    Limit* limit = orderB->parentLimit; // Uses the getter we added to Order.h

    // Check if Limit's head is now B
    // Uses the getter we added to Limit.h
    EXPECT_EQ(limit->head, orderB);

    // Order B should now be the front (prev is null)
    EXPECT_EQ(orderB->prevOrder, nullptr);
}

TEST_F(OrderBookTest, EmptyPriceLevel_IsRemovedAfterFill) {
    book.addLimitOrder(1, 100, 10, Side::SELL);
    book.addLimitOrder(2, 100, 10, Side::BUY);

    EXPECT_EQ(getAskDepth(), 0);
    EXPECT_EQ(getBidDepth(), 0);
}