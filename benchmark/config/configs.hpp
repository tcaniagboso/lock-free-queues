#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace benchmark::config {
    struct BenchmarkConfig {
        std::span<const size_t> capacities_;
        std::span<const uint64_t> operation_counts_;
        std::span<const size_t> producer_counts_;
        std::span<const size_t> consumer_counts_;
        uint64_t warmup_ops_;
    };

    inline constexpr std::array<size_t,4> capacities{
            256, 1024, 4096, 16384
    };

    inline constexpr std::array<uint64_t,3> ops{
            100000, 1000000, 5000000
    };

    inline constexpr uint64_t warmup_ops = 200000;

    inline constexpr std::array<size_t,1> one_thread{1};
    inline constexpr std::array<size_t,3> many_threads{2,4,8};

    inline constexpr BenchmarkConfig spsc_config{
            capacities,
            ops,
            one_thread,
            one_thread,
            warmup_ops
    };

    inline constexpr BenchmarkConfig mpsc_config{
            capacities,
            ops,
            many_threads,
            one_thread,
            warmup_ops
    };

    inline constexpr BenchmarkConfig spmc_config{
            capacities,
            ops,
            one_thread,
            many_threads,
            warmup_ops
    };

    inline constexpr BenchmarkConfig mpmc_config{
            capacities,
            ops,
            many_threads,
            many_threads,
            warmup_ops
    };
} // namespace benchmark::config