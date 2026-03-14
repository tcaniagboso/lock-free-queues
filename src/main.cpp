#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#include "spsc_queue.hpp"

struct Stats {
    uint64_t p50_latency_ns_;
    uint64_t p95_latency_ns_;
    uint64_t p999_latency_ns_;
    uint64_t min_latency_ns_;
    uint64_t max_latency_ns_;
    double avg_latency_ns_;

    Stats(uint64_t p50_latency_ns, uint64_t p95_latency_ns, uint64_t p999_latency_ns, uint64_t min_latency_ns,
          uint64_t max_latency_ns, double avg_latency_ns)
            : p50_latency_ns_{p50_latency_ns},
              p95_latency_ns_{p95_latency_ns},
              p999_latency_ns_{p999_latency_ns},
              min_latency_ns_{min_latency_ns},
              max_latency_ns_{max_latency_ns},
              avg_latency_ns_{avg_latency_ns} {}

    void print(const std::string &operation) const {
        std::cout << operation << ":" << std::endl;
        std::cout << "\tAvg latency (ns): " << avg_latency_ns_ << std::endl;
        std::cout << "\tp50 latency (ns): " << p50_latency_ns_ << std::endl;
        std::cout << "\tp95 latency (ns): " << p95_latency_ns_ << std::endl;
        std::cout << "\tp999 latency (ns): " << p999_latency_ns_ << std::endl;
        std::cout << "\tMin latency (ns): " << min_latency_ns_ << std::endl;
        std::cout << "\tMax latency (ns): " << max_latency_ns_ << std::endl;
    }

};

using clk = std::chrono::steady_clock;
using ns = std::chrono::nanoseconds;

void producer_worker(spsc::SPSCQueue<int> &spsc_queue, uint64_t num_operations, std::vector<uint64_t> &latencies) {
    for (uint64_t i = 0; i < num_operations; i++) {
        int value = i % 256;
        uint64_t latency;
        while (true) {
            auto start = clk::now();
            if (spsc_queue.try_push(value)) {
                auto end = clk::now();
                latency = std::chrono::duration_cast<ns>(end - start).count();
                break;
            }
        }

        latencies[i] = latency;
    }
}

void consumer_worker(spsc::SPSCQueue<int> &spsc_queue, uint64_t num_operations, std::vector<uint64_t> &latencies) {
    for (uint64_t i = 0; i < num_operations; i++) {
        int value;
        uint64_t latency;
        while (true) {
            auto start = clk::now();
            if (spsc_queue.try_pop(value)) {
                auto end = clk::now();
                latency = std::chrono::duration_cast<ns>(end - start).count();
                break;
            }
        }

        latencies[i] = latency;
    }
}

Stats compute_stats(std::vector<uint64_t> &latencies, uint64_t num_operations) {
    std::sort(latencies.begin(), latencies.end());
    long double sum_latency = 0.0;
    for (uint64_t i = 0; i < num_operations; i++) {
        sum_latency += latencies[i];
    }

    size_t p50_index = num_operations / 2;
    size_t p95_index = num_operations * 95 / 100;
    size_t p999_index = num_operations * 999 / 1000;

    double avg_latency_ns = static_cast<double>(sum_latency) / static_cast<double>(num_operations);
    uint64_t min_latency_ns = latencies.front();
    uint64_t max_latency_ns = latencies.back();
    uint64_t p50_latency_ns = latencies[p50_index];
    uint64_t p95_latency_ns = latencies[p95_index];
    uint64_t p999_latency_ns = latencies[p999_index];

    return {
            p50_latency_ns,
            p95_latency_ns,
            p999_latency_ns,
            min_latency_ns,
            max_latency_ns,
            avg_latency_ns
    };
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cout << "Usage:" << std::endl;
        std::cout << argv[0] << " [capacity] [# push operations] [# pop operations]" << std::endl;
        return -1;
    }

    // Parsing
    size_t capacity = std::stoull(argv[1]);
    size_t num_push = std::stoull(argv[2]);
    size_t num_pop = std::stoull(argv[3]);

    spsc::SPSCQueue<int> spsc_queue{capacity};

    // Latency arrays
    std::vector<uint64_t> push_latencies(num_push, std::numeric_limits<uint64_t>::max());
    std::vector<uint64_t> pop_latencies(num_pop, std::numeric_limits<uint64_t>::max());

    // Operations
    auto start = clk::now();
    std::thread producer(&producer_worker, std::ref(spsc_queue), num_push, std::ref(push_latencies));
    std::thread consumer(&consumer_worker, std::ref(spsc_queue), num_pop, std::ref(pop_latencies));
    consumer.join();
    producer.join();
    auto end = clk::now();

    // Benchmarking
    double total_time = std::chrono::duration<double>(end - start).count();
    double throughput = static_cast<double>(num_push + num_pop) / total_time;
    Stats push_stats = compute_stats(push_latencies, num_push);
    Stats pop_stats = compute_stats(pop_latencies, num_pop);

    std::cout << "Throughput: " << throughput << std::endl;
    std::cout << std::endl;
    push_stats.print("PUSH");
    std::cout << std::endl;
    pop_stats.print("POP");

    return 0;
}
