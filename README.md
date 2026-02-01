# High-Performance Limit Order Book (LOB)

A low-latency, single-threaded matching engine implemented in C++20. This project simulates the core infrastructure of a financial exchange, prioritizing memory layout optimization, instruction cache locality, and algorithmic efficiency.

It implements a **Hybrid Architecture** combining `std::map` for price level discovery with **Intrusive Linked Lists** for constant-time order management.

## 🛠 Architecture

## 1. Switch From std::map To std::vector (Data Structures)

### The Problem: std:map

Traditional matching engines rely on `std::map` (Red-Black Trees).

* **Memory Layout:** Nodes are allocated individually on the heap, leading to scattered memory. A node at address `0x1000` might point to a child at `0x9040`.
* **The Cost:** To traverse the tree, the CPU must chase pointers. Each hop to a random memory address causes a **Cache Miss** (~100 CPU cycles wasted waiting for data from DRAM).
* **Result:** Latency is dominated by memory fetch times, not calculation speed.

### Optimized Solution: O(1) Direct Indexing

We abandon trees entirely in favor of a pre-allocated `std::vector` where the **index** corresponds directly to the **price**.

* **Impact:**
* **Contiguous Memory:** `std::vector` allocates one giant block of memory. Price 100, 101, and 102 are physically next to each other in RAM. Thus when the CPU loads Price 100 into its cache, the prefetcher automatically pulls in Price 101, 102, and 103.
* **Instant Access:** Lookup complexity is **constant time** O(1), regardless of book depth.
* **Potential Drawback:**
* **Sparse Price Levels** If price levels are sparse, then we will have to traverse the array in order to find a valid order. This is addressed in the next optimization.


---

## 2. Hardware-Accelerated Bitmasks


The single biggest source of latency in a Limit Order Book is finding the "Next Best Price" when the top level is depleted.

**The Problem: Sparseness**
Order books are sparse. If the Best Bid is at **100.00** and the next order is at **99.00**, a standard naive array scan must check `99.99`, `99.98` ... `99.01`—performing **100 memory reads** just to find empty air.

**The Solution: Hierarchical Bitmaps**
We overlay a `std::vector<uint64_t>` bitset on top of the price array. Each bit represents the presence of orders at a specific price level.

Instead of looping, we use **Compiler Intrinsics** to map search logic directly to CPU silicon:

* **Asks (Scanning Up):** We use `__builtin_ctzll` (Count Trailing Zeros Long Long).
* *Hardware Action:* Maps to the `TZCNT` instruction. The CPU finds the **lowest set bit** (lowest price) in a single clock cycle.


* **Bids (Scanning Down):** We use `__builtin_clzll` (Count Leading Zeros Long Long).
* *Hardware Action:* Maps to the `LZCNT` instruction. The CPU finds the **highest set bit** (highest price) in a single clock cycle.



**Hierarchical Skipping**
To handle large universes (e.g., 100,000 ticks), the engine checks validity at the **Block Level** (64 ticks) before checking the **Price Level**.

```cpp
// 1. Check if an entire block of 64 prices is empty (Single integer comparison)
if (masks[blockIndex] == 0) {
    blockIndex--; // SKIPPED 64 PRICES IN ~1 CYCLE
    continue;
}

// 2. If not empty, find the exact bit instantly using hardware intrinsics
// Note: We use 'll' variant for explicit 64-bit width
int bitPos = 63 - __builtin_clzll(masks[blockIndex]);

```

**Performance Impact:**
| Operation | Naive Loop | M3 Bitmask | Speedup |
| :--- | :--- | :--- | :--- |
| **Skip 60 Empty Prices** | 60 Iterations | 1 Comparison | **~60x** |
| **Find Active Price** | Linear Search | `LZCNT` Instruction | **O(1)** |

---

## 3. The Order Chain (Intrusive List)

### The Problem: Slow Queue

* **Standard List (`std::list`)**: When you put an object into a standard list, the list allocates a new node wrapper on the heap that points to your data.

* **Result**: 
* **Extra Memory** You have two memory allocations: one for your Order object, and one for the Node wrapper. 
* **Pointer Indirection** Standard `std::list<Order*>` requires triple indirection (List Node → Smart Pointer → Order).
* **Cache Misses** They are likely far apart in RAM. To access the order, the CPU has to load the Node, read the pointer, and then jump to the Order (**Double Cache Miss**).

### Optimized Solution: Intrusive Linked List

You **embed** the `next` and `prev` pointers inside the Order struct itself.

* **Result**: The Order is the node. There is no external **wrapper**.

* **Impact**: 
* **No Extra Memory Allocations:** When you have a pointer to an Order, you immediately have the pointers to the `next` Order. It is extremely **cache-friendly**.
* **`O(1)` Removal:** Since the Order contains its own `prev` and `next` pointers, it can literally remove itself from the chain without asking the list container.

```text
[ Bids ]
    |
    +-- $101.00 (Limit) 
    |      |
    |      +-- [Order #1] <--> [Order #2] <--> [Order #3]
    |
    +-- $100.00 (Limit)
           |
           +-- [Order #4] <--> [Order #5]

```

---

## 4. Zero-Allocation Hot Path (Memory Management)

### The Problem: Slow Memory Allocation

Dynamic memory allocation (`new`, `malloc`) is the standard method for memory allocation in c++, but it is way too slow for HFT purposes.

* **Non-Deterministic** The search for open memory can take anywhere from `10ns` to `10,000ns`, these spikes are referred to as `Jitters`
* **"Cold" Memory** The OS might give you a memory address that hasn't been touched in a while, meaning the CPU has to wait` ~100ns` to fetch it from **DRAM**.
* **Memory Fragmentations** Free memory is broken up into scattered non-contiguous blocks
* **System Calls** Can trigger a **Context Switch** into **Kernel Mode**, which effectively pauses your program to let the OS do administrative work.

This process introduces unpredictable latency spikes that ruin P99 performance.

### Optimized Solution: The Slab Allocator

We implemented a custom **Object Pool** that pre-allocates a massive contiguous block of memory ("Slab") at startup.

* **LIFO Recycling:** Deleted orders are pushed onto a simple stack. When a new order creates a need for an object, we pop the most recently deleted one.
* **Impact:**
* **Determinism** You pre-allocate all the memory at startup, so it's the same memory being used throughout the session, preventing `jitters`.
* **Zero System Calls:** The Engine never interacts with the OS kernel during the trading phase.
* **Imporved Cache Locality** When one object is accessed, nearby objects are also loaded into the CPU's cache, significantly reducing access latency compared to a fresh allocation.


---

## 5. Branch Hoisting (Pipeline Optimization)

### The Problem: Branch Misprediction

Modern CPUs rely on deep pipelines to execute instructions. When the code hits an `if/else` statement (branch), the CPU must guess which path to take. If it guesses wrong, it must "flush" the pipeline, discarding partially executed work and wasting ~15-20 cycles.

### Optimized Solution: Pointer Arithmetic

We "hoist" branches out of the hot loop. Instead of checking `if (side == BUY)` millions of times inside the matching loop, we use pointer arithmetic to select the data structures **once**.

```cpp
// Branch logic executed ONCE.
// The CPU pipeline can now unroll the loop without misprediction fears.
auto& opposingBook = (side == Side::BUY) ? asks : bids;
Price* bestPrice   = (side == Side::BUY) ? &lowestAsk : &highestBid;

// The loop contains NO conditional logic regarding 'Side'
while (qty > 0) {
    // Direct pointer access
    Limit* limit = opposingBook[*bestPrice]; 
    ...
}

```


## 📊 Performance Benchmarks

This engine includes a dedicated deterministic benchmark harness (`Benchmark.cpp`) capable of simulating millions of orders to measure **Tick-to-Trade** latency and **Throughput**.

### Methodology

* **Environment:** Apple M3 Macbook Air
* **Measurements:** * **Throughput:** Orders processed per second (including matching, partial fills, and cancellations).
* **Latency:** Nanosecond-precision timestamping from "Order Submission" to "Trade Callback Execution."
* **Workload:** 2,000,000 Orders
* **Distribution:** 70% Limit / 25% Cancel / 5% Market
* **Warmup:** 100,000 cycle warmup phase to prime the Branch Predictor and Instruction Cache.

### ⚡ Performance Evolution

| Metric | Standard Implementation | M3 Optimized Engine | Improvement |
| --- | --- | --- | --- |
| **Throughput (Pure)** | ~8,500,000 ops/sec | **~65,541,839 ops/sec** | **🚀 7.6x Higher** |
| **Throughput (Tracked)** | ~6,400,000 ops/sec | **~34,108,471 ops/sec** | **🚀 5.3x Higher** |
| **Median Latency (P50)** | ~42 ns | **~41 ns** | **~2% Faster** |
| **P90 Latency** | ~458 ns | **~42 ns** | **⚡ 10.9x Faster** |
| **P99 Latency** | ~15,000 ns (15 μs) | **~84 ns** | **⚡ 178x Faster** |


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

For the most accurate results on macOS, run the benchmark with elevated task policy to prevent CPU throttling:

```bash
# Throughput Mode
./src/run_benchmark

# Latency Mode (Includes P50/P99 stats)
./src/run_benchmark --latency

```

### 3. Run Unit Tests

Verifies core matching logic, edge cases (empty book, self-match), and order lifecycle.

```bash
./tests/OrderBookTests

```

## 🔮 Future Improvements

1. **SIMD Vectorization:** Use AVX-512 to process order matching in batches.
2. **Lock-Free Concurrency:** Replace the single-threaded model with a Ring Buffer to handle network I/O on a separate thread.


## 📄 License

This project is licensed under the MIT License - see the [LICENSE](https://www.google.com/search?q=LICENSE) file for details.