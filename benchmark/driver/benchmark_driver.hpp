#include <array>
#include <cstdint>

#include "spsc/queue.hpp"
#include "experiments/lockfree_queue_experiment.hpp"
#include "experiments/mutex_queue_experiment.hpp"
#include "payload/payload.hpp"

namespace benchmark {
    constexpr std::array<size_t, 4> capacities{
            256,
            1024,
            4096,
            16384
    };

    constexpr std::array<uint64_t, 3> operation_counts{
            100000,
            1000000,
            5000000
    };

    constexpr std::array<size_t, 1> producer_counts{
            1,
    };

    constexpr std::array<size_t, 1> consumer_counts{
            1,
    };

    constexpr uint64_t warmup_ops = 200000;

    template<typename Payload>
    void run_payload_benchmarks() {
        for (const auto capacity: capacities) {
            for (const auto ops: operation_counts) {
                auto push_ops = ops;
                auto pop_ops = ops;
                for (const auto producers: producer_counts) {
                    for (const auto consumers: consumer_counts) {
                        std::cout << std::string(50, '=') << '\n';
                        std::cout << "Payload Size: " << sizeof(Payload) << '\n';
                        std::cout << "Capacity: " << capacity << '\n';
                        std::cout << "Producers: " << producers << '\n';
                        std::cout << "Consumers: " << consumers << '\n';
                        std::cout << "Push Operations: " << push_ops << '\n';
                        std::cout << "Pop Operations: " << pop_ops << '\n';
                        std::cout << std::string(50, '=') << '\n';

                        LockFreeQueueExperiment<lock_free::spsc::Queue<Payload>, Payload> lf_experiment{
                                producers,
                                consumers,
                                capacity,
                                warmup_ops,
                                push_ops,
                                pop_ops
                        };

                        lf_experiment.warmup();
                        lf_experiment.run_benchmark();
                        lf_experiment.compute_and_print_stats();
                        std::cout << '\n';

                        benchmark::MutexQueueExperiment<Payload> mtx_experiment{
                                producers,
                                consumers,
                                capacity,
                                warmup_ops,
                                push_ops,
                                pop_ops
                        };
                        mtx_experiment.warmup();
                        mtx_experiment.run_benchmark();
                        mtx_experiment.compute_and_print_stats();
                        std::cout << "\n\n";
                    }
                }
            }
        }
    }

    template<typename... Payloads>
    void run_payload_tests() {
        (run_payload_benchmarks<Payloads>(), ...);
    }

    void run_benchmark_suite() {
        run_payload_tests<Payload16, Payload64, Payload256>();
    }
} // namespace benchmark