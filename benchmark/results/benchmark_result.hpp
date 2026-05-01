#pragma once

#include <cstdint>
#include <string>

#include "../stats/stats.hpp"

namespace benchmark::results {
    using namespace benchmark::stats;
    struct BenchmarkResult {
        std::string queue_;
        std::string policy_;

        double throughput_;

        Stats push_stats_;
        Stats pop_stats_;
    };
} // namespace benchmark::result