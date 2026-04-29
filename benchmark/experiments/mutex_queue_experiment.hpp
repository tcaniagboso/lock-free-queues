#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>

#include "stats.hpp"

namespace benchmark {
    using clk = std::chrono::steady_clock;
    using ns = std::chrono::nanoseconds;

    template<typename Payload>
    class MutexQueueExperiment {
    private:
        std::queue<Payload> queue_;
        std::mutex mtx_;
        std::condition_variable cv_;
        size_t num_producers_;
        size_t num_consumers_;
        size_t capacity_;
        uint64_t warmup_ops_;
        uint64_t push_ops_;
        uint64_t pop_ops_;
        std::vector<uint64_t> push_latencies_;
        std::vector<uint64_t> pop_latencies_;
        double total_seconds_;

        Stats compute_stats(uint64_t ops, std::vector<uint64_t> &latencies);

        void producer_worker(uint64_t ops, uint64_t offset, bool record_latency = true);

        void consumer_worker(uint64_t ops, uint64_t offset, bool record_latency = true);

        void run_operations(uint64_t push_ops, uint64_t pop_ops, bool record_latency);

    public:
        MutexQueueExperiment(size_t num_producers, size_t num_consumers, size_t capacity, uint64_t warmup_ops,
                             uint64_t push_ops, uint64_t pop_ops);

        void warmup();

        void run_benchmark();

        void compute_and_print_stats();
    };

    template<typename Payload>
    benchmark::MutexQueueExperiment<Payload>::MutexQueueExperiment(size_t num_producers, size_t num_consumers,
                                                                   size_t capacity, uint64_t warmup_ops,
                                                                   uint64_t push_ops, uint64_t pop_ops)
            : queue_{},
              mtx_{},
              cv_{},
              num_producers_{num_producers},
              num_consumers_{num_consumers},
              capacity_{capacity},
              warmup_ops_{warmup_ops},
              push_ops_{push_ops},
              pop_ops_{pop_ops},
              push_latencies_(push_ops),
              pop_latencies_(pop_ops),
              total_seconds_{} {}

    template<typename Payload>
    Stats benchmark::MutexQueueExperiment<Payload>::compute_stats(uint64_t ops, std::vector<uint64_t> &latencies) {
        std::sort(latencies.begin(), latencies.end());
        long double sum_latency = 0.0;
        for (uint64_t i = 0; i < ops; i++) {
            sum_latency += latencies[i];
        }

        size_t p50_index = ops / 2;
        size_t p95_index = ops * 95 / 100;
        size_t p999_index = ops * 999 / 1000;

        double avg_latency_ns = static_cast<double>(sum_latency) / static_cast<double>(ops);
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

    template<typename Payload>
    void benchmark::MutexQueueExperiment<Payload>::producer_worker(uint64_t ops, uint64_t offset, bool record_latency) {
        Payload p;
        for (uint64_t i = 0; i < ops; i++) {
            p.id = offset + i;
            if (record_latency) {
                const auto start = clk::now();
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait(lock, [&]() {
                    return queue_.size() < capacity_;
                });
                queue_.push(p);
                const auto end = clk::now();
                lock.unlock();
                cv_.notify_one();
                uint64_t latency = std::chrono::duration_cast<ns>(end - start).count();
                push_latencies_[offset + i] = latency;
            } else {
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait(lock, [&]() {
                    return queue_.size() < capacity_;
                });
                queue_.push(p);
                lock.unlock();
                cv_.notify_one();
            }
        }
    }

    template<typename Payload>
    void benchmark::MutexQueueExperiment<Payload>::consumer_worker(uint64_t ops, uint64_t offset, bool record_latency) {
        Payload p;
        for (uint64_t i = 0; i < ops; i++) {
            if (record_latency) {
                const auto start = clk::now();
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait(lock, [&]() {
                    return !queue_.empty();
                });
                p = queue_.front();
                queue_.pop();
                const auto end = clk::now();
                lock.unlock();
                cv_.notify_one();
                uint64_t latency = std::chrono::duration_cast<ns>(end - start).count();
                pop_latencies_[offset + i] = latency;
            } else {
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait(lock, [&]() {
                    return !queue_.empty();
                });
                p = queue_.front();
                queue_.pop();
                lock.unlock();
                cv_.notify_one();
            }
        }
    }

    template<typename Payload>
    void benchmark::MutexQueueExperiment<Payload>::run_operations(uint64_t push_ops, uint64_t pop_ops,
                                                                  bool record_latency) {
        std::vector<std::thread> producers{};
        std::vector<std::thread> consumers{};

        producers.reserve(num_producers_);
        consumers.reserve(num_consumers_);

        using Self = MutexQueueExperiment<Payload>;
        uint64_t base_push_ops = num_producers_ ? push_ops / num_producers_ : 0;
        uint64_t base_pop_ops = num_consumers_ ? pop_ops / num_consumers_ : 0;

        // Producers
        for (size_t i = 0; i < num_producers_; i++) {
            uint64_t offset = i * base_push_ops;
            uint64_t ops = base_push_ops;

            if (i == num_producers_ - 1) ops = push_ops - offset;

            producers.emplace_back(
                    &Self::producer_worker,
                    this,
                    ops,
                    offset,
                    record_latency
            );
        }

        // Consumers
        for (size_t i = 0; i < num_consumers_; i++) {
            uint64_t offset = i * base_pop_ops;
            uint64_t ops = base_pop_ops;

            if (i == num_consumers_ - 1) ops = pop_ops - offset;

            consumers.emplace_back(
                    &Self::consumer_worker,
                    this,
                    ops,
                    offset,
                    record_latency
            );
        }

        for (auto &p: producers) p.join();
        for (auto &c: consumers) c.join();
    }

    template<typename Payload>
    void benchmark::MutexQueueExperiment<Payload>::warmup() {
        run_operations(warmup_ops_, warmup_ops_, false);
    }

    template<typename Payload>
    void benchmark::MutexQueueExperiment<Payload>::run_benchmark() {
        const auto start = clk::now();
        run_operations(push_ops_, pop_ops_, true);
        const auto end = clk::now();
        total_seconds_ = std::chrono::duration<double>(end - start).count();
    }

    template<typename Payload>
    void benchmark::MutexQueueExperiment<Payload>::compute_and_print_stats() {
        auto total_ops = static_cast<double>(push_ops_ + pop_ops_);
        double throughput = total_ops / total_seconds_;
        Stats push_stats = compute_stats(push_ops_, push_latencies_);
        Stats pop_stats = compute_stats(pop_ops_, pop_latencies_);

        std::cout << "[Mutex Queue]" << '\n';
        std::cout << "Throughput (ops/sec): " << throughput << '\n';
        push_stats.print("PUSH");
        std::cout << '\n';
        pop_stats.print("POP");
    }
} // namespace benchmark