#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

#include "../config/configs.hpp"
#include "../experiments/lockfree_queue_experiment.hpp"
#include "../experiments/mutex_queue_experiment.hpp"
#include "../payload/payload.hpp"
#include "../policies/queue_policies.hpp"
#include "../results/csv_writer.hpp"

namespace benchmark::driver {
    using namespace benchmark::config;
    using namespace benchmark::experiments;
    using namespace benchmark::payload;
    using namespace benchmark::policies;
    using namespace benchmark::results;

    template<template<typename> class Queue, typename PushPolicy, typename PopPolicy, typename Payload>
    void run_mode(const BenchmarkSetup &setup, CsvWriter &writer) {
        static_assert(
                std::string_view(PushPolicy::name()) ==
                std::string_view(PopPolicy::name())
        );
        LockFreeQueueExperiment<Queue<Payload>, Payload, PushPolicy, PopPolicy> lf_experiment{
                setup.producers_,
                setup.consumers_,
                setup.capacity_,
                setup.warmup_ops_,
                setup.push_ops_,
                setup.pop_ops_
        };

        lf_experiment.warmup();
        lf_experiment.run_benchmark();
        BenchmarkResult lf_result = lf_experiment.compute_result();
        lf_result.policy_ = PushPolicy::name();
        lf_experiment.print_result(lf_result);
        writer.write(setup, lf_result);
        std::cout << '\n';
    }

    template<typename Payload>
    void run_mutex(const BenchmarkSetup &setup, CsvWriter &writer) {
        MutexQueueExperiment<Payload> mtx_experiment{
                setup.producers_,
                setup.consumers_,
                setup.capacity_,
                setup.warmup_ops_,
                setup.push_ops_,
                setup.pop_ops_
        };

        mtx_experiment.warmup();
        mtx_experiment.run_benchmark();
        BenchmarkResult mtx_result = mtx_experiment.compute_result();
        mtx_result.policy_ = "Baseline";
        mtx_experiment.print_result(mtx_result);
        writer.write(setup, mtx_result);
        std::cout << "\n";
    }

    template<template<typename> class Queue, typename Payload>
    void run_payload_benchmarks(const BenchmarkConfig &config, CsvWriter &writer) {
        for (const auto capacity: config.capacities_) {
            for (const auto ops: config.operation_counts_) {
                auto push_ops = ops;
                auto pop_ops = ops;
                for (const auto producers: config.producer_counts_) {
                    for (const auto consumers: config.consumer_counts_) {
                        std::cout << std::string(50, '=') << '\n';
                        std::cout << "Queue: " << Queue<Payload>::name() << '\n';
                        std::cout << "Payload Size: " << sizeof(Payload) << '\n';
                        std::cout << "Capacity: " << capacity << '\n';
                        std::cout << "Producers: " << producers << '\n';
                        std::cout << "Consumers: " << consumers << '\n';
                        std::cout << "Push Operations: " << push_ops << '\n';
                        std::cout << "Pop Operations: " << pop_ops << '\n';
                        std::cout << std::string(50, '=') << '\n';

                        BenchmarkSetup setup{
                                push_ops,
                                pop_ops,
                                config.warmup_ops_,
                                sizeof(Payload),
                                capacity,
                                producers,
                                consumers
                        };

                        run_mode<Queue, TryPush, TryPop, Payload>(setup, writer);
                        run_mode<Queue, BlockingPush, BlockingPop, Payload>(setup, writer);
                        run_mutex<Payload>(setup, writer);

                        std::cout << "\n";

                    }
                }
            }
        }
    }

    template<template<typename> class Queue, typename... Payloads>
    void run_payload_tests(const BenchmarkConfig &config, CsvWriter &writer) {
        (run_payload_benchmarks<Queue, Payloads>(config, writer), ...);
    }

    template<template<typename> class Queue>
    void run_benchmark_suite(const BenchmarkConfig &config, const std::string &csv_path) {
        CsvWriter writer{csv_path};
        run_payload_tests<Queue, Payload16, Payload64, Payload256>(config, writer);
    }
} // namespace benchmark::driver