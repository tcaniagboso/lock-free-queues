#pragma once

#include <chrono>
#include <fstream>
#include <filesystem>
#include <string>

#include "benchmark_result.hpp"
#include "benchmark_setup.hpp"

namespace benchmark::results {

    class CsvWriter {
    private:
        std::ofstream file_;
        std::string timestamp_;

        static std::string make_timestamp() {
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);

            std::stringstream ss;
            ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");

            return ss.str();
        }

    public:
        explicit CsvWriter(const std::string& path)
                : file_(path, std::ios::app)
        {
            timestamp_ = make_timestamp();
            std::filesystem::create_directories(
                    std::filesystem::path(path).parent_path()
            );

            if (file_.tellp() == 0) {
                file_
                        << "timestamp,"
                        << "queue,policy,payload_size,capacity,producers,consumers,push_ops,pop_ops,warmup_ops,"
                        << "throughput,"
                        << "push_avg_ns,push_p50_ns,push_p95_ns,push_p999_ns,push_min_ns,push_max_ns,"
                        << "pop_avg_ns,pop_p50_ns,pop_p95_ns,pop_p999_ns,pop_min_ns,pop_max_ns\n";
            }
        }

        void write(const BenchmarkSetup& setup, const BenchmarkResult& result) {
            file_
                    << timestamp_ << ','

                    << result.queue_ << ','
                    << result.policy_ << ','

                    << setup.payload_size_ << ','
                    << setup.capacity_ << ","
                    << setup.producers_ << ","
                    << setup.consumers_ << ","
                    << setup.push_ops_ << ","
                    << setup.pop_ops_ << ","
                    << setup.warmup_ops_ << ","

                    << result.throughput_ << ','

                    << result.push_stats_.avg_latency_ns_ << ','
                    << result.push_stats_.p50_latency_ns_ << ','
                    << result.push_stats_.p95_latency_ns_ << ','
                    << result.push_stats_.p999_latency_ns_ << ','
                    << result.push_stats_.min_latency_ns_ << ','
                    << result.push_stats_.max_latency_ns_ << ','

                    << result.pop_stats_.avg_latency_ns_ << ','
                    << result.pop_stats_.p50_latency_ns_ << ','
                    << result.pop_stats_.p95_latency_ns_ << ','
                    << result.pop_stats_.p999_latency_ns_ << ','
                    << result.pop_stats_.min_latency_ns_ << ','
                    << result.pop_stats_.max_latency_ns_

                    << '\n';
        }
    };
} // namespace benchmark::results