#include "Book.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <pthread.h>
#include <random>
#include <vector>

#ifdef __linux__
#include <sched.h>
#include <unistd.h>
#endif

const int ORDER_COUNT = 2'000'000;
const int MAX_ORDERS = 10'000'000;
const int ITERATIONS = 10;

static std::int64_t timestamps[MAX_ORDERS + 1];

struct OrderAction {
    OrderId id;
    Price price;
    Quantity qty;
    OrderType type;
    Side side;
};

void pinThreadToCore([[maybe_unused]] int core_id) {
#if defined(__linux__)
    // LINUX: Strict Pinning (Locks thread to specific CPU ID)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_t current_thread = pthread_self();
    int rc = pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);

    if (rc != 0) {
        std::cerr << "[Linux] Warning: Failed to pin to Core " << core_id << "\n";
    } else {
        std::cout << "[Linux] Optimization: Thread pinned to Core " << core_id << "\n";
    }
#elif defined(__APPLE__)
    // Hints scheduler to use performance cores over efficiency cores
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    std::cout << "[macOS] Optimization: QoS set to USER_INTERACTIVE (Performance Cores)\n";
#else
    // WINDOWS / OTHER
    std::cout << "[System] Optimization: Pinning not supported on this OS.\n";
#endif
}

class BenchmarkRunner {
private:
    std::vector<long long> latencies;
    bool measureLatency = false;

    std::vector<double> statsThroughput, statsP50, statsP90, statsP99, statsMax;

    std::string formatNum(long long n) {
        std::string s = std::to_string(n);
        int insertPosition = s.length() - 3;
        while (insertPosition > 0) {
            s.insert(insertPosition, ",");
            insertPosition -= 3;
        }
        return s;
    }

public:
    void setMeasureLatency(bool val) { measureLatency = val; }

    void run(const std::vector<OrderAction>& actions, int iteration) {
        Book book(ORDER_COUNT + 1000, 100000);
        ;

        // Trade Callback for Tick to Trade Latency
        if (measureLatency) {
            latencies.clear();
            latencies.reserve(actions.size());

            book.setTradeCallback([&](const Trade& t) {
                auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();

                std::int64_t start = timestamps[t.takerOrderId];

                if (start > 0) {
                    latencies.push_back(now - start);
                }
            });
        }

        // --- Warmup Phase ---
        {
            Book warmupBook(100000, 1000);

            for (int i = 0; i < 1'000'000; ++i) {
                warmupBook.addLimitOrder(i, 10000 + (i % 10), 1, Side::BUY);
                warmupBook.cancelOrder(i);
            }
        }

        // --- Benchmark Phase ---
        auto startTime = std::chrono::steady_clock::now();

        for (const auto& order : actions) {
            if (measureLatency) {
                timestamps[order.id] = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                           std::chrono::steady_clock::now().time_since_epoch())
                                           .count();
            }

            switch (order.type) {
            case OrderType::LIMIT:
                book.addLimitOrder(order.id, order.price, order.qty, order.side);
                break;
            case OrderType::CANCEL:
                book.cancelOrder(order.id);
                break;
            case OrderType::MARKET:
                book.addMarketOrder(order.id, order.qty, order.side);
                break;
            }
        }

        auto endTime = std::chrono::steady_clock::now();

        // --- Reporting ---

        // Throughput
        std::chrono::duration<double> duration = endTime - startTime;
        long long tput = static_cast<long long>(actions.size() / duration.count());

        statsThroughput.push_back(tput);

        std::cout << "Iteration " << std::setw(2) << iteration << " | Time: " << std::fixed << std::setprecision(3)
                  << duration.count() << "s"
                  << " | Tput: " << std::setw(9) << tput << " ops/s";

        // Latency
        if (measureLatency && !latencies.empty()) {
            std::sort(latencies.begin(), latencies.end());

            size_t n = latencies.size();
            long long p50 = latencies[n * 0.50];
            long long p90 = latencies[n * 0.90];
            long long p99 = latencies[n * 0.99];
            long long maxLat = latencies.back();

            statsP50.push_back(p50);
            statsP90.push_back(p90);
            statsP99.push_back(p99);
            statsMax.push_back(maxLat);

            std::cout << " | Latency(ns) [Median: " << p50 << " | P90: " << p90 << " | P99: " << p99
                      << " | Max: " << maxLat << "]";

        } else if (measureLatency) {
            std::cout << " | [No Trades Occurred]";
        }
        std::cout << '\n';
    }

    void printSummary() {
        if (statsThroughput.empty())
            return;

        auto computeAvg = [](const std::vector<double>& v) {
            if (v.empty())
                return 0.0;
            return std::reduce(v.begin(), v.end(), 0.0) / v.size();
        };

        long long avgTput = static_cast<long long>(computeAvg(statsThroughput));

        std::cout << "\n============================================\n";
        std::cout << "             BENCHMARK SUMMARY              \n";
        std::cout << "============================================\n";
        std::cout << "Orders Per Run    : " << formatNum(ORDER_COUNT) << " Orders \n";
        std::cout << "Total Runs        : " << statsThroughput.size() << "\n";
        std::cout << "Avg Throughput    : " << formatNum(avgTput) << " ops/sec\n";

        if (measureLatency && !statsP50.empty()) {
            long long avgP50 = static_cast<long long>(computeAvg(statsP50));
            long long avgP90 = static_cast<long long>(computeAvg(statsP90));
            long long avgP99 = static_cast<long long>(computeAvg(statsP99));
            long long avgMax = static_cast<long long>(computeAvg(statsMax));

            std::cout << "--------------------------------------------\n";
            std::cout << "P50 Latency   : " << formatNum(avgP50) << " ns\n";
            std::cout << "P90 Latency   : " << formatNum(avgP90) << " ns\n";
            std::cout << "P99 Latency   : " << formatNum(avgP99) << " ns\n";
            std::cout << "Max Latency   : " << formatNum(avgMax) << " ns\n";
        }
        std::cout << "============================================\n";
    }
};

std::vector<OrderAction> pregenerate(int count) {
    std::vector<OrderAction> actions;
    actions.reserve(count);
    // Seed RNG
    std::mt19937 rng(42);

    // 70% Limit Order, 25 Cancel Order, 5 Market Order
    std::discrete_distribution<int> typeDist({70, 25, 5});
    // Range from [$99.70, $100.30]
    std::normal_distribution<double> priceDist(10000.0, 30.0);
    // 50/50 Buy/Sell
    std::uniform_int_distribution<int> sideDist(0, 1);
    // Skewed right (close to real-world)
    std::lognormal_distribution<double> qtyDist(3.0, 0.5);

    std::vector<OrderId> activeIds;
    OrderId curId = 1;

    for (int i = 0; i < count; i++) {
        OrderType type = static_cast<OrderType>(typeDist(rng));
        Side side = (sideDist(rng) == 0) ? Side::BUY : Side::SELL;
        Quantity qty = static_cast<Quantity>(std::max(1.0, qtyDist(rng)));

        if (type == OrderType::LIMIT || activeIds.empty()) {
            // Limit Order
            Price p = static_cast<Price>(priceDist(rng));
            actions.push_back({curId, p, qty, OrderType::LIMIT, side});
            activeIds.push_back(curId++);
        } else if (type == OrderType::MARKET) {
            // Market Order
            actions.push_back({curId++, 0, qty, type, side});
        } else if (type == OrderType::CANCEL) {
            // Cancel Order
            size_t idx = std::uniform_int_distribution<size_t>(0, activeIds.size() - 1)(rng);
            actions.push_back({activeIds[idx], 0, 0, OrderType::CANCEL, Side::BUY});

            activeIds[idx] = activeIds.back();
            activeIds.pop_back();
        }
    }

    return actions;
}

int main(int argc, char* argv[]) {
    // Pin cores if possible
    pinThreadToCore(0);
    // Benchmark Latency is toggleable
    bool latencyMode = false;
    for (int i = 0; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--latency" || arg == "-l") {
            latencyMode = true;
        }
    };

    std::cout << "Pre-generating " << ORDER_COUNT << " actions...\n";
    auto actions = pregenerate(ORDER_COUNT);

    BenchmarkRunner runner;
    runner.setMeasureLatency(latencyMode);

    std::cout << "Running benchmark...\n";
    if (latencyMode) {
        std::cout << "Latency Tracking Enabled \n";
    }

    for (int i = 0; i < ITERATIONS; i++) {
        runner.run(actions, i);
    }

    runner.printSummary();

    return 0;
}
