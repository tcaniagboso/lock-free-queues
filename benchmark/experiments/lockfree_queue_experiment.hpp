#pragma once

#include <chrono>
#include <cstdint>
#include <vector>
#include <thread>

#include "../results/benchmark_result.hpp"

namespace benchmark::experiments {
    using clk = std::chrono::steady_clock;
    using ns = std::chrono::nanoseconds;
    using namespace benchmark::results;

    template<typename Queue, typename Payload, typename PushPolicy, typename PopPolicy>
    class LockFreeQueueExperiment {
    private:
        Queue queue_;
        PushPolicy push_policy_;
        PopPolicy pop_policy_;
        uint64_t warmup_ops_;
        uint64_t push_ops_;
        uint64_t pop_ops_;
        size_t num_producers_;
        size_t num_consumers_;
        size_t capacity_;
        std::vector<uint64_t> push_latencies_;
        std::vector<uint64_t> pop_latencies_;
        double total_seconds_;

        Stats compute_stats(uint64_t ops, std::vector<uint64_t> &latencies);

        void producer_worker(uint64_t ops, uint64_t offset, bool record_latency = true);

        void consumer_worker(uint64_t ops, uint64_t offset, bool record_latency = true);

        void run_operations(uint64_t push_ops, uint64_t pop_ops, bool record_latency);


    public:
        LockFreeQueueExperiment(size_t num_producers, size_t num_consumers, size_t capacity, uint64_t warmup_ops,
                                uint64_t push_ops, uint64_t pop_ops);

        void warmup();

        void run_benchmark();

        [[nodiscard]] BenchmarkResult compute_result();

        static void print_result(const BenchmarkResult& result);

    };

    template<typename Queue, typename Payload, typename PushPolicy, typename PopPolicy>
    LockFreeQueueExperiment<Queue, Payload, PushPolicy, PopPolicy>::LockFreeQueueExperiment(size_t num_producers,
                                                                                size_t num_consumers,
                                                                                size_t capacity, uint64_t warmup_ops,
                                                                                uint64_t push_ops, uint64_t pop_ops)
            : queue_{capacity},
              push_policy_{},
              pop_policy_{},
              warmup_ops_{warmup_ops},
              push_ops_{push_ops},
              pop_ops_{pop_ops},
              num_producers_{num_producers},
              num_consumers_{num_consumers},
              capacity_{capacity},
              push_latencies_(push_ops),
              pop_latencies_(pop_ops),
              total_seconds_{0} {}

    template<typename Queue, typename Payload, typename PushPolicy, typename PopPolicy>
    Stats LockFreeQueueExperiment<Queue, Payload, PushPolicy, PopPolicy>::compute_stats(uint64_t ops, std::vector<uint64_t> &latencies) {
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

    template<typename Queue, typename Payload, typename PushPolicy, typename PopPolicy>
    void LockFreeQueueExperiment<Queue, Payload, PushPolicy, PopPolicy>::producer_worker(uint64_t ops, uint64_t offset,
                                                                             bool record_latency) {
        Payload p;
        for (uint64_t i = 0; i < ops; i++) {
            p.id = offset + i;
            if (record_latency) {
                const auto start = clk::now();
                push_policy_(queue_, p);
                const auto end = clk::now();
                uint64_t latency = std::chrono::duration_cast<ns>(end - start).count();
                push_latencies_[offset + i] = latency;
            } else {
                push_policy_(queue_, p);
            }
        }
    }

    template<typename Queue, typename Payload, typename PushPolicy, typename PopPolicy>
    void LockFreeQueueExperiment<Queue, Payload, PushPolicy, PopPolicy>::consumer_worker(uint64_t ops, uint64_t offset,
                                                                             bool record_latency) {
        Payload p;
        for (uint64_t i = 0; i < ops; i++) {
            if (record_latency) {
                const auto start = clk::now();
                pop_policy_(queue_, p);
                const auto end = clk::now();
                uint64_t latency = std::chrono::duration_cast<ns>(end - start).count();
                pop_latencies_[offset + i] = latency;
            } else {
                pop_policy_(queue_, p);
            }
        }
    }

    template<typename Queue, typename Payload, typename PushPolicy, typename PopPolicy>
    void LockFreeQueueExperiment<Queue, Payload, PushPolicy, PopPolicy>::run_operations(uint64_t push_ops, uint64_t pop_ops,
                                                                            bool record_latency) {
        std::vector<std::thread> producers{};
        std::vector<std::thread> consumers{};

        producers.reserve(num_producers_);
        consumers.reserve(num_consumers_);

        using Self = LockFreeQueueExperiment<Queue, Payload, PushPolicy, PopPolicy>;
        uint64_t base_push_ops = num_producers_ ? push_ops / num_producers_ : 0;
        uint64_t base_pop_ops = num_consumers_ ? pop_ops / num_consumers_ : 0;

        // Producers
        for (size_t i = 0; i < num_producers_; i++) {
            uint64_t offset = i * base_push_ops;
            uint64_t ops = base_push_ops;

            if (i == num_producers_ - 1) {
                ops = push_ops - offset;
            }

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

            if (i == num_consumers_ - 1) {
                ops = pop_ops - offset;
            }

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

    template<typename Queue, typename Payload, typename PushPolicy, typename PopPolicy>
    void LockFreeQueueExperiment<Queue, Payload, PushPolicy, PopPolicy>::warmup() {
        run_operations(warmup_ops_, warmup_ops_, false);
    }

    template<typename Queue, typename Payload, typename PushPolicy, typename PopPolicy>
    void LockFreeQueueExperiment<Queue, Payload, PushPolicy, PopPolicy>::run_benchmark() {
        const auto start = clk::now();
        run_operations(push_ops_, pop_ops_, true);
        const auto end = clk::now();
        total_seconds_ = std::chrono::duration<double>(end - start).count();
    }

    template<typename Queue, typename Payload, typename PushPolicy, typename PopPolicy>
    BenchmarkResult LockFreeQueueExperiment<Queue, Payload, PushPolicy, PopPolicy>::compute_result() {
        auto total_ops = static_cast<double>(push_ops_ + pop_ops_);
        double throughput = total_ops / total_seconds_;

        BenchmarkResult result;

        result.queue_ = Queue::name();
        result.policy_ = PushPolicy::name();
        result.throughput_ = throughput;

        result.push_stats_ = compute_stats(push_ops_, push_latencies_);
        result.pop_stats_  = compute_stats(pop_ops_, pop_latencies_);

        return result;
    }

    template<typename Queue, typename Payload, typename PushPolicy, typename PopPolicy>
    void LockFreeQueueExperiment<Queue, Payload, PushPolicy, PopPolicy>::print_result(const BenchmarkResult& result) {
        std::cout << '[' << result.queue_ << ']' << '\n';
        std::cout << "Throughput (ops/sec): " << result.throughput_ << '\n';
        result.push_stats_.print("PUSH");
        std::cout << '\n';
        result.pop_stats_.print("POP");
    }
} // namespace benchmark::experiments