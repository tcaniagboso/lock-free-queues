#pragma once

#include <cstdint>
#include <iostream>
#include <string>

namespace benchmark {
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
            std::cout << "\tAvg latency (ns): " << avg_latency_ns_ << '\n';
            std::cout << "\tp50 latency (ns): " << p50_latency_ns_ << '\n';
            std::cout << "\tp95 latency (ns): " << p95_latency_ns_ << '\n';
            std::cout << "\tp999 latency (ns): " << p999_latency_ns_ << '\n';
            std::cout << "\tMin latency (ns): " << min_latency_ns_ << '\n';
            std::cout << "\tMax latency (ns): " << max_latency_ns_ << '\n';
        }
    };
} // namespace benchmark