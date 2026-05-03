#include <filesystem>
#include <iostream>
#include <string_view>

#include "../benchmark/config/configs.hpp"
#include "../benchmark/driver/benchmark_driver.hpp"
#include "../include/mpmc/queue.hpp"
#include "../include/mpsc/queue.hpp"
#include "../include/spmc/queue.hpp"
#include "../include/spsc/queue.hpp"

using namespace lock_free;
using namespace benchmark;

void print_help() {
    std::cout << "Usage: ./bench [OPTIONS]\n\n";

    std::cout << "Options:\n";
    std::cout << "  --all            Benchmark ALL queues\n";
    std::cout << "  --mpmc           Benchmark MPMC Queue\n";
    std::cout << "  --mpsc           Benchmark MPSC Queue\n";
    std::cout << "  --spmc           Benchmark SPMC Queue\n";
    std::cout << "  --spsc           Benchmark SPSC Queue\n";
    std::cout << "  -h, --help       Show this help message\n\n";

    std::cout << "Example:\n";
    std::cout << "  ./bench --spsc --spmc\n";
}

int main(int argc, char *argv[]) {
    namespace fs = std::filesystem;

    const fs::path root = PROJECT_ROOT;

    bool all{false};
    bool spsc{false};
    bool spmc{false};
    bool mpsc{false};
    bool mpmc{false};

    if (argc == 1) {
        all = true;
    } else {
        for (int i = 1; i < argc; i++) {
            std::string_view arg = argv[i];
            if (arg == "-h" || arg == "--help") {
                print_help();
                return 0;
            } else if (arg == "--spsc") {
                spsc = true;
            } else if (arg == "--spmc") {
                spmc = true;
            } else if (arg == "--mpsc") {
                mpsc = true;
            } else if (arg == "--mpmc") {
                mpmc = true;
            } else if (arg == "--all") {
                all = true;
                break;
            } else {
                std::cerr << "Invalid argument: " << arg << "\n";
                print_help();
                return 1;
            }

            if (spsc && spmc && mpsc && mpmc) {
                all = true;
                break;
            }
        }
    }

    spsc = spsc || all;
    spmc = spmc || all;
    mpsc = mpsc || all;
    mpmc = mpmc || all;

    std::cout << "Lock-Free Queue Benchmark Suite\n";
    std::cout << "Writing results to /results\n\n";

    if (spsc) {
        driver::run_benchmark_suite<spsc::Queue>(
                config::spsc_config,
                (root / "results" / "spsc.csv").string()
        );
    }

    if (spmc) {
        driver::run_benchmark_suite<spmc::Queue>(
                config::spmc_config,
                (root / "results" / "spmc.csv").string()
        );
    }

    if (mpsc) {
        driver::run_benchmark_suite<mpsc::Queue>(
                config::mpsc_config,
                (root / "results" / "mpsc.csv").string()
        );
    }

    if (mpmc) {
        driver::run_benchmark_suite<mpmc::Queue>(
                config::mpmc_config,
                (root / "results" / "mpmc.csv").string()
        );
    }

    return 0;
}