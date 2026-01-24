# High-Performance Limit Order Book (LOB)

A low-latency, single-threaded matching engine implemented in C++20. This project simulates the core infrastructure of a financial exchange, prioritizing memory layout optimization, instruction cache locality, and algorithmic efficiency.

It implements a **Hybrid Architecture** combining `std::map` for price level discovery with **Intrusive Linked Lists** for constant-time order management.

## 🚀 Key Features

* **Intrusive Data Structures:** Orders contain their own `next`/`prev` pointers, eliminating the need for `std::list` container overhead and improving cache locality.
* **O(1) Cancellations:** Achieving constant-time removal of resting orders via direct pointer manipulation, bypassing tree traversals.
* **Memory Aligned:** Critical data structures (`Order`, `Limit`) are packed and aligned to 32 bytes to fit exactly two objects per 64-byte CPU cache line.
* **Price-Time Priority:** Strict FIFO execution at every price level.
* **Aggressive Matching:** Supports Market and Limit orders with immediate spread-crossing capabilities.

## 🛠 Architecture

### 1. The Price Skeleton (`std::map`)

* Price levels are stored in a Red-Black Tree (`std::map`).
* **Why?** While searching for a price is $O(\log N)$, the number of active price levels is significantly smaller than the number of orders. This provides a balance of safety and speed.

### 2. The Order Chain (Intrusive List)

* Each `Limit` object acts as a header for a doubly linked list.
* **Why?** Standard `std::list<Order*>` requires triple indirection (List Node  Smart Pointer  Order). By embedding pointers inside the `Order` struct, we achieve **zero-allocation insertion** and better memory adjacency.

```text
[ Bids Map ]
    |
    +-- $101.00 (Limit) 
    |      |
    |      +-- [Order #1] <--> [Order #2] <--> [Order #3]
    |
    +-- $100.00 (Limit)
           |
           +-- [Order #4] <--> [Order #5]

```

### 3. The Lookup Table (`unordered_map`)

* Maps `OrderId -> Order*`.
* Enables  access to any resting order. Because orders are intrusive, we can unlink them from their `Limit` parent instantly without searching the list.

## ⚡ Performance Optimization

This engine implements several HFT-style optimizations:

* **Struct Padding & Alignment:** The `Limit` class is manually padded to 32 bytes, preventing false sharing and ensuring efficient cache line utilization.
* **Branch Prediction:** Hot paths (matching logic) are separated from cold paths (cancellation/error handling).
* **Raw Pointers:** Usage of raw pointers (`Order*`) instead of `std::shared_ptr` to avoid reference counting overhead on the hot path.

## 📦 Building and Running

### Prerequisites

* C++20 compatible compiler (GCC 10+, Clang 11+, MSVC)
* [CMake](https://cmake.org/) (3.10+)

### Compilation

```bash
# Clone the repository
git clone https://github.com/yourusername/Limit-Order-Book.git
cd Limit-Order-Book

# Create build directory
mkdir build && cd build

# Configure and Build
cmake ..
make

```

## 🧪 Testing & Validation

This project employs a three-tiered testing strategy to ensure both logical correctness and low-latency performance.

1. Unit Tests (Google Test)

Verifies the core logic of the matching engine, order lifecycle, and edge cases.

* **Matching Logic:** Validates that market orders correctly "walk the book" (match against multiple levels) and handle partial fills.
* **Order Operations:** atomic Add/Cancel/Modify operations.
* **Edge Cases:** Tests empty books, self-matching prevention, and zero-quantity handling.

```bash
# Run the functional test suite
./build/tests/unit_tests

```


## 🗺 Roadmap

* [x] Hybrid Architecture (Map + Intrusive List)
* [x] O(1) Order Cancellation
* [x] Memory Layout Optimization (32-byte alignment)
* [ ] **Object Pooling:** Implement a slab allocator to replace `new`/`delete` and eliminate heap fragmentation.
* [ ] **Lock-Free Concurrency:** Investigate `std::atomic` for a multi-threaded matching engine.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](https://www.google.com/search?q=LICENSE) file for details.


Here is an expanded **Testing & Benchmarking** section for your `README.md`. You can replace the existing "Running Tests" section with this detailed breakdown.

This version highlights the rigorousness of your testing (Latency, Correctness, and Stress), which is very attractive to recruiters or users looking at HFT projects.

---
