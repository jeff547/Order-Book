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
* **Why?** While searching for a price is , the number of active price levels is significantly smaller than the number of orders. This provides a balance of safety and speed.

### 2. The Order Chain (Intrusive List)

* Each `Limit` object acts as a header for a doubly linked list.
* **Why?** Standard `std::list<Order*>` requires triple indirection (List Node → Smart Pointer → Order). By embedding pointers inside the `Order` struct, we achieve **zero-allocation insertion** and better memory adjacency.

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

### 3. The Lookup Table (`std::unordered_map`)

* Maps `OrderId -> Order*`.
* Enables  access to any resting order. Because orders are intrusive, we can unlink them from their `Limit` parent instantly without searching the list.

## 📊 Performance Benchmarks

This engine includes a dedicated deterministic benchmark harness (`Benchmark.cpp`) capable of simulating millions of orders to measure **Tick-to-Trade** latency and **Throughput**.

### Methodology

* **Environment:** Apple M3 Pro (Performance Cores Pinned).
* **Measurements:** * **Throughput:** Orders processed per second (including matching, partial fills, and cancellations).
* **Latency:** Nanosecond-precision timestamping from "Order Submission" to "Trade Callback Execution."


* **Warmup:** 1,000,000 cycle warmup phase to prime the Branch Predictor and Instruction Cache.

### Current Results (2M Order Microbenchmark)

The engine currently sustains **~8.5 million operations per second** with sub-microsecond tail latency on cached datasets.

| Metric | Result | 
| --- | --- | 
| **Throughput** | **~8,500,000 ops/sec** | 
| **Median Latency** | **42 ns**  |
| **P99 Latency** | **~458 ns**  |
| **Max Latency** | **~15 μs**  |


## 📦 Building and Running

### Prerequisites

* C++20 compatible compiler (GCC 10+, Clang 11+, MSVC)
* [CMake](https://cmake.org/) (3.14+)
* **GoogleTest** (for unit testing)

### 1. Build (Release Mode)

**Critical:** You must build in `Release` mode to enable `-O3` and `-march=native` optimizations. 

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make

```

### 2. Run Benchmarks

Run the harness with the latency flag enabled to see the P99 histograms.

```bash
# Run 10 Million Order Benchmark with Latency Tracking (Remove --latency for just tput results)
./src/run_benchmark --latency

```

### 3. Run Unit Tests

Verifies core matching logic, edge cases (empty book, self-match), and order lifecycle.

```bash
./tests/OrderBookTests

```

## 🗺 Roadmap

* [x] Hybrid Architecture (Map + Intrusive List)
* [x] O(1) Order Cancellation
* [x] Memory Layout Optimization (32-byte alignment)
* [x] **High-Precision Benchmarking:** Implemented Latency/Throughput measurement with CPU pinning.
* [ ] **Object Pooling:** Implement a slab allocator to replace `new`/`delete` and eliminate heap fragmentation.


## 📄 License

This project is licensed under the MIT License - see the [LICENSE](https://www.google.com/search?q=LICENSE) file for details.