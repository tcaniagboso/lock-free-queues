#pragma once

#include <cstdint>
#include <cstddef>

namespace benchmark::results {
    struct BenchmarkSetup {
        uint64_t push_ops_;
        uint64_t pop_ops_;
        uint64_t warmup_ops_;
        size_t payload_size_;
        size_t capacity_;
        size_t producers_;
        size_t consumers_;
    };
} // namespace benchmark::results