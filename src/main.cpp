#include <algorithm>
#include <condition_variable>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <mutex>
#include <queue>
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

void lf_producer_worker(lock_free::SPSCQueue<int> &lf_queue, uint64_t num_operations, std::vector<uint64_t> &latencies) {
    for (uint64_t i = 0; i < num_operations; i++) {
        int value = i % 256;
        uint64_t latency;
        while (true) {
            auto start = clk::now();
            if (lf_queue.try_push(value)) {
                auto end = clk::now();
                latency = std::chrono::duration_cast<ns>(end - start).count();
                break;
            }
        }

        latencies[i] = latency;
    }
}

void lf_consumer_worker(lock_free::SPSCQueue<int> &spsc_queue, uint64_t num_operations, std::vector<uint64_t> &latencies) {
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

void mtx_producer_worker(std::queue<int> &mtx_queue, uint64_t num_operations, std::vector<uint64_t> &latencies,
                         uint64_t capacity, std::mutex &mtx, std::condition_variable &cv) {
    for (uint64_t i = 0; i < num_operations; i++) {
        int val = i % 256;
        auto start = clk::now();
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() {
            return mtx_queue.size() < capacity;
        });
        mtx_queue.push(val);
        lock.unlock();
        cv.notify_one();
        auto end = clk::now();

        uint64_t latency = std::chrono::duration_cast<ns>(end - start).count();
        latencies[i] = latency;
    }
}

void mtx_consumer_worker(std::queue<int> &mtx_queue, uint64_t num_operations, std::vector<uint64_t> &latencies,
                         std::mutex &mtx, std::condition_variable &cv) {
    for (uint64_t i = 0; i < num_operations; i++) {
        auto start = clk::now();
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() {
            return !mtx_queue.empty();
        });
        int val = mtx_queue.front();
        mtx_queue.pop();
        lock.unlock();
        cv.notify_one();
        auto end = clk::now();
        uint64_t latency = std::chrono::duration_cast<ns>(end - start).count();
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

    lock_free::SPSCQueue<int> lf_queue{capacity};
    std::queue<int> mtx_queue{};
    std::mutex mtx{};
    std::condition_variable cv{};

    // Latency arrays
    std::vector<uint64_t> lf_push_latencies(num_push, std::numeric_limits<uint64_t>::max());
    std::vector<uint64_t> lf_pop_latencies(num_pop, std::numeric_limits<uint64_t>::max());
    std::vector<uint64_t> mtx_push_latencies(num_push, std::numeric_limits<uint64_t>::max());
    std::vector<uint64_t> mtx_pop_latencies(num_pop, std::numeric_limits<uint64_t>::max());

    // warm_up

    // Operations
    // lock_free  queue
    auto lf_start = clk::now();
    std::thread lf_producer(&lf_producer_worker, std::ref(lf_queue), num_push, std::ref(lf_push_latencies));
    std::thread lf_consumer(&lf_consumer_worker, std::ref(lf_queue), num_pop, std::ref(lf_pop_latencies));
    lf_consumer.join();
    lf_producer.join();
    auto lf_end = clk::now();

    // mutex queue
    auto mtx_start = clk::now();
    std::thread mtx_producer(&mtx_producer_worker, std::ref(mtx_queue), num_push, std::ref(mtx_push_latencies),
                             capacity, std::ref(mtx), std::ref(cv));
    std::thread mtx_consumer(&mtx_consumer_worker, std::ref(mtx_queue), num_pop, std::ref(mtx_pop_latencies),
                             std::ref(mtx), std::ref(cv));
    mtx_producer.join();
    mtx_consumer.join();
    auto mtx_end = clk::now();

    // Benchmarking
    uint64_t total_ops = num_push + num_pop;
    double lf_total_time = std::chrono::duration<double>(lf_end - lf_start).count();
    double lf_throughput = static_cast<double>(total_ops) / lf_total_time;
    Stats lf_push_stats = compute_stats(lf_push_latencies, num_push);
    Stats lf_pop_stats = compute_stats(lf_pop_latencies, num_pop);

    std::cout << "Lock Free Queue Throughput: " << lf_throughput << std::endl;
    std::cout << std::endl;
    lf_push_stats.print("PUSH");
    std::cout << std::endl;
    lf_pop_stats.print("POP");

    double mtx_total_time = std::chrono::duration<double>(mtx_end - mtx_start).count();
    double mtx_throughput = static_cast<double>(total_ops) / mtx_total_time;
    Stats mtx_push_stats = compute_stats(mtx_push_latencies, num_push);
    Stats mtx_pop_stats = compute_stats(mtx_pop_latencies, num_pop);

    std::cout << "\nMutex Queue Throughput: " << mtx_throughput << std::endl;
    std::cout << std::endl;
    mtx_push_stats.print("PUSH");
    std::cout << std::endl;
    mtx_pop_stats.print("POP");

    return 0;
}
